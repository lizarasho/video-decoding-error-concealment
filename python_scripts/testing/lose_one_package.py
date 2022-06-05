import math
import os
import subprocess
from pathlib import Path
from typing import List

import cv2
import matplotlib.pyplot as plt
import numpy as np
from scipy.misc import imresize

from testing.settings.common import T_APP_DECODER, LOSE_BITS, ADDITIONAL_INFO, RESOLUTION, ERROR_ALGO_PARAMS
from testing.settings.lose_packages import DAMAGED_STREAMS, RECONSTRUCTED_STREAMS, ONE_PKG_LOST, \
    ORIGINAL_YUVS, ORIGINAL_STREAMS, NAL_UNITS_DESC, FIGURES_PATH
from utils.common import calc_bytes_size
from utils.yuv_utils import VideoCaptureYUV


def downscale_video(input_yuv_path, input_resolution, output_yuv_path, output_resolution, frames_number, start_from=0):
    output_frame_size = output_resolution[0] * output_resolution[1] * 3 // 2
    cap = VideoCaptureYUV(input_yuv_path, input_resolution[1], input_resolution[0])
    output_file = open(output_yuv_path, 'wb')
    cap.skip_frames(start_from)

    for current_frame in range(0, frames_number):
        ret, source_yuv_frame = cap.read()
        if ret:
            source_yuv_frame = source_yuv_frame.reshape(cap.shape)
            source_bgr_frame = cv2.cvtColor(source_yuv_frame, cv2.COLOR_YUV2BGR_I420)
            assert source_bgr_frame.shape == input_resolution + (3,)

            output_bgr_frame = imresize(source_bgr_frame, output_resolution)
            assert output_bgr_frame.shape == output_resolution + (3,)

            output_yuv_frame = cv2.cvtColor(output_bgr_frame, cv2.COLOR_BGR2YUV_I420)
            output_yuv_frame = np.array(output_yuv_frame).tobytes()
            assert output_frame_size == len(output_yuv_frame)

            output_file.write(output_yuv_frame)
            current_frame += 1
        else:
            break
        assert calc_bytes_size(output_yuv_path) == output_frame_size * current_frame

    output_file.close()


def calc_ideal_metric(clip_name, qp, metric):
    width, height = RESOLUTION[clip_name]
    yuv_desc = f'-s:v {width}x{height} -c:v rawvideo -pix_fmt yuv420p'
    command = f'ffmpeg ' \
              f'{yuv_desc} -i {ORIGINAL_YUVS}/{clip_name}.yuv ' \
              f'{yuv_desc} -i {ORIGINAL_STREAMS}/{clip_name}_QP_{qp}.yuv ' \
              f'-lavfi {metric} -f null -'
    process = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return process.stderr.splitlines()[-1].split()[4:]


# algo_version \in ['base', 'nn']
def calc_algo_metric(clip_name, qp, i, iteration, metric, algo_version):
    width, height = RESOLUTION[clip_name]
    yuv_desc = f'-s:v {width}x{height} -c:v rawvideo -pix_fmt yuv420p'
    command = f'ffmpeg ' \
              f'{yuv_desc} -i {ORIGINAL_YUVS}/{clip_name}.yuv ' \
              f'{yuv_desc} -i {RECONSTRUCTED_STREAMS}/{clip_name}_QP_{qp}/iteration_{iteration}/{algo_version}_algo/{i}.yuv ' \
              f'-lavfi {metric} -f null -'
    process = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return process.stderr.splitlines()[-1].split()[4:]


def split_into_PSNRs(metric_output):
    psnrs = list(map(lambda x: float(x.decode('utf-8').split(':')[1]), metric_output))
    y_psnr, u_psnr, v_psnr, avg_psnr, min_psnr, max_psnr = psnrs
    return y_psnr, u_psnr, v_psnr, avg_psnr, min_psnr, max_psnr


def calc_ideal_psnrs(clip_name, qp):
    metric_output = calc_ideal_metric(clip_name, qp, 'psnr')
    return split_into_PSNRs(metric_output)


def calc_algo_PSNRs(clip_name, qp, i, iteration, algo_version):
    metric_output = calc_algo_metric(clip_name, qp, i, iteration, 'psnr', algo_version)
    return split_into_PSNRs(metric_output)


def construct_nal_packages(clip_name):
    original_packages = f'{NAL_UNITS_DESC}/original'
    if not os.path.exists(original_packages):
        os.makedirs(original_packages)

    damaged_packages = f'{ONE_PKG_LOST}/{clip_name}/iteration_1'
    if not os.path.exists(damaged_packages):
        os.makedirs(damaged_packages)

    with open(f'{original_packages}/{clip_name}.txt', 'r') as f:
        packages_lines = f.readlines()

    for i in range(len(packages_lines)):
        with open(f'{damaged_packages}/{i}.txt', 'w') as g:
            packages_lost = packages_lines.copy()
            packages_lost.pop(i)
            g.write(''.join(packages_lost))


def lose_bits(clip_name):
    with open(f'{NAL_UNITS_DESC}/original/{clip_name}.txt', 'r') as f:
        packages_lines = f.readlines()

    damaged_packages = f'{ONE_PKG_LOST}/{clip_name}/iteration_1'
    damaged_streams = f'{DAMAGED_STREAMS}/{clip_name}/iteration_1'

    if not os.path.exists(damaged_streams):
        os.makedirs(damaged_streams)

    for i in range(0, len(packages_lines)):
        original_stream = f'{ORIGINAL_STREAMS}/{clip_name}.h265'
        damaged_stream = f'{damaged_streams}/{i}.h265'
        nal_units_desc = f'{damaged_packages}/{i}.txt'

        command = f'{LOSE_BITS} {original_stream} {nal_units_desc} {damaged_stream}'
        print(command)
        os.system(command)


def decode(clip_name, base_algo=4):
    with open(f'{NAL_UNITS_DESC}/original/{clip_name}.txt', 'r') as f:
        packages_lines = f.readlines()

    reconstructed_streams = f'{RECONSTRUCTED_STREAMS}/{clip_name}/iteration_1/base_algo'
    if not os.path.exists(reconstructed_streams):
        os.makedirs(reconstructed_streams)

    for i in range(0, len(packages_lines)):
        damaged_stream = f'{DAMAGED_STREAMS}/{clip_name}/iteration_1/{i}.h265'
        reconstructed_stream = f'{reconstructed_streams}/{i}.yuv'
        params = ERROR_ALGO_PARAMS[base_algo]
        command = f'{T_APP_DECODER} -b {damaged_stream} -o {reconstructed_stream} -a {base_algo} {params}'
        os.system(command)


class Point:
    def __init__(self, indices, bytes_size, PSNR, dPSNR):
        self.indices = indices
        self.bytes_size = bytes_size
        self.PSNR = PSNR
        self.dPSNR = dPSNR


def filter_good_points(points: List[Point]) -> List[Point]:
    points = list(filter(lambda p: p.dPSNR > 1e-4, points))
    points.sort(key=lambda p: (-p.dPSNR, p.indices))
    return points


def get_top_points(points: List[Point], top_n: int = 100000) -> List[Point]:
    points.sort(key=lambda p: (-p.dPSNR, p.indices))
    return points[:top_n]


def calc_bitrate(bytes_size, fps=24, frames_number=24):
    return fps * 8 * bytes_size / (1000 * frames_number)


# algo_version \in ['size', 'bitrate']
def calc_good_results(source_clip_name, main_qp, all_qps, title=None, plot_mode='bitrate', base_algo=4):
    original_sizes = []
    original_psnrs = []

    if title is None:
        title = source_clip_name

    print('ideal:')
    for qp in all_qps:
        original_size = calc_bytes_size(f'{ORIGINAL_STREAMS}/{source_clip_name}_QP_{qp}.h265')
        original_psnr, _, _, _, _, _ = calc_ideal_psnrs(source_clip_name, qp)
        original_sizes.append(original_size)
        original_psnrs.append(float(original_psnr))
        print(qp, original_size, original_psnr)

    deg = len(original_sizes) - 1
    poly = np.polyfit(original_sizes, original_psnrs, deg)
    current_best_points = []
    current_gain = 0

    cycle_colors = plt.rcParams['axes.prop_cycle'].by_key()['color']
    plt.title(title, fontweight='bold', color='#30302f')
    sizes_linspace = np.linspace(min(original_sizes), max(original_sizes), 500)
    psnrs_linspace = [np.polyval(poly, i) for i in sizes_linspace]
    if plot_mode == 'bitrate':
        bitrates_linspace = list(map(lambda x: calc_bitrate(x), sizes_linspace))
        plt.plot(bitrates_linspace, psnrs_linspace, color=cycle_colors[0], label='Without errors')
    elif plot_mode == 'size':
        plt.plot(sizes_linspace, psnrs_linspace, color=cycle_colors[0], label='Without errors')

    clip_name_qp = f'{source_clip_name}_QP_{main_qp}'

    with open(f'{NAL_UNITS_DESC}/original/{clip_name_qp}.txt', 'r') as f:
        packages_lines = f.readlines()

    # Iteration 1, FS
    iteration = 1
    tested_points: List[Point] = []
    for i in range(3, len(packages_lines)):
        damaged_stream = f'{DAMAGED_STREAMS}/{clip_name_qp}/iteration_1/{i}.h265'

        damaged_size = calc_bytes_size(damaged_stream)
        y_psnr_damaged, _, _, _, _, _ = calc_algo_PSNRs(source_clip_name, main_qp, i, 1, 'base')
        y_psnr_original = np.polyval(poly, damaged_size)

        point = Point([i], damaged_size, y_psnr_damaged, y_psnr_damaged - y_psnr_original)
        tested_points.append(point)

        print(point.indices, point.bytes_size, point.PSNR)

    good_points = filter_good_points(tested_points)
    current_best_points = get_top_points(current_best_points + good_points, 50)
    print('The best 50 points after 1st iteration:')
    for point in current_best_points:
        print(point.indices, point.bytes_size, point.PSNR, point.dPSNR)
    good_sizes = list(map(lambda p: p.bytes_size, good_points))
    good_psnrs = list(map(lambda p: p.PSNR, good_points))
    xlims = [min(good_sizes), max(good_sizes)]
    ylims = [min(good_psnrs), max(good_psnrs)]
    if plot_mode == 'bitrate':
        good_bitrates = list(map(lambda x: calc_bitrate(x), good_sizes))
        plt.scatter(good_bitrates, good_psnrs, color=cycle_colors[iteration],
                    label=f'Frame Stitching, {iteration} iteration', s=1)
    elif plot_mode == 'size':
        plt.scatter(good_sizes, good_psnrs, color=cycle_colors[iteration],
                    label=f'Frame Stitching, {iteration} iteration', s=1)
    iteration += 1

    # Iterations > 1, FS
    while current_best_points[0].dPSNR - current_gain > 1e-4:
        current_gain = current_best_points[0].dPSNR
        print(f'Current gain after iteration {iteration - 1} equals {current_gain}. Starting iteration {iteration}...')

        packages_folder = f'{ONE_PKG_LOST}/{clip_name_qp}/iteration_{iteration}'

        if not os.path.exists(packages_folder):
            os.makedirs(packages_folder)
        if not os.path.exists(f'{DAMAGED_STREAMS}/{clip_name_qp}/iteration_{iteration}'):
            os.makedirs(f'{DAMAGED_STREAMS}/{clip_name_qp}/iteration_{iteration}')
        if not os.path.exists(f'{RECONSTRUCTED_STREAMS}/{clip_name_qp}/iteration_{iteration}/base_algo'):
            os.makedirs(f'{RECONSTRUCTED_STREAMS}/{clip_name_qp}/iteration_{iteration}/base_algo')

        tested_points: List[Point] = []
        for i in range(len(current_best_points)):
            for j in range(i + 1, len(current_best_points)):
                merged_indices = list(set(current_best_points[i].indices + current_best_points[j].indices))
                merged_indices.sort()
                if any(point.indices == merged_indices for point in tested_points) | \
                        any(point.indices == merged_indices for point in current_best_points):
                    continue

                file_name = '+'.join(map(str, merged_indices))

                if not os.path.exists(f'{packages_folder}/{file_name}.txt'):
                    with open(f'{packages_folder}/{file_name}.txt', 'w') as g:
                        packages_lost = packages_lines.copy()
                        for index in reversed(merged_indices):
                            packages_lost.pop(index)
                        g.write(''.join(packages_lost))

                damaged_stream = f'{DAMAGED_STREAMS}/{clip_name_qp}/iteration_{iteration}/{file_name}.h265'
                if not os.path.exists(damaged_stream):
                    original_stream = f'{ORIGINAL_STREAMS}/{clip_name_qp}.h265'
                    nal_units_desc = f'{ONE_PKG_LOST}/{clip_name_qp}/iteration_{iteration}/{file_name}.txt'
                    command_lose_bits = f'{LOSE_BITS} {original_stream} {nal_units_desc} {damaged_stream}'
                    os.system(command_lose_bits)

                reconstructed_stream = f'{RECONSTRUCTED_STREAMS}/{clip_name_qp}/iteration_{iteration}/base_algo/{file_name}.yuv'
                if not os.path.exists(reconstructed_stream):
                    params = ERROR_ALGO_PARAMS[base_algo]
                    command_decode = f'{T_APP_DECODER} -b {damaged_stream} -o {reconstructed_stream} -a {base_algo} {params}'
                    os.system(command_decode)

                damaged_size = calc_bytes_size(damaged_stream)
                y_psnr_damaged, _, _, _, _, _ = calc_algo_PSNRs(source_clip_name, main_qp, file_name, iteration, 'base')
                y_psnr_original = np.polyval(poly, damaged_size)

                point = Point(merged_indices, damaged_size, y_psnr_damaged, y_psnr_damaged - y_psnr_original)
                tested_points.append(point)

                print(point.indices, point.bytes_size, point.PSNR)

        good_points = filter_good_points(tested_points)

        if len(good_points) != 0:
            current_best_points = get_top_points(current_best_points + good_points, 60)
            print(f'Top-60 good points after {iteration} iteration:')
            for point in current_best_points:
                print(point.indices, point.bytes_size, point.PSNR, point.dPSNR)

            good_sizes = list(map(lambda p: p.bytes_size, good_points))
            good_psnrs = list(map(lambda p: p.PSNR, good_points))
            xlims = [min(xlims[0], min(good_sizes)), max(xlims[1], max(good_sizes))]
            ylims = [min(ylims[0], min(good_psnrs)), max(ylims[1], max(good_psnrs))]

            if plot_mode == 'bitrate':
                good_bitrates = list(map(lambda x: calc_bitrate(x), good_sizes))
                plt.scatter(good_bitrates, good_psnrs, color=cycle_colors[iteration],
                            label=f'Frame Stitching, {iteration} iteration', s=1)
            elif plot_mode == 'size':
                plt.scatter(good_sizes, good_psnrs, color=cycle_colors[iteration],
                            label=f'Frame Stitching, {iteration} iteration', s=1)
        else:
            print(f'On the {iteration} there were no good points')

        iteration += 1

    # Max iteration, NN
    reconstructed_streams = f'{RECONSTRUCTED_STREAMS}/{clip_name_qp}'

    iterations = ([str(f.stem) for f in Path(reconstructed_streams).iterdir() if f.is_dir()])
    iterations = list(map(lambda x: int(x.split('_')[1]), iterations))
    max_iter = np.max(iterations)
    print(f'Max iteration is {max_iter}.')
    if len(os.listdir(f'{reconstructed_streams}/iteration_{max_iter}/base_algo')) == 0:
        print(f'Iteration {max_iter} folder is empty. Walk through iteration {max_iter - 1}.')
        max_iter -= 1

    print(f'Calculate for NN on iterations {max_iter - 1} ... {max_iter}')
    nn_points = []
    for iteration in range(max_iter - 1, max_iter + 1):
        base_dir_iter = f'{reconstructed_streams}/iteration_{iteration}/base_algo'
        nn_dir_iter = f'{reconstructed_streams}/iteration_{iteration}/nn_algo'

        if not os.path.exists(nn_dir_iter):
            os.makedirs(nn_dir_iter)

        for file_name in os.listdir(base_dir_iter):
            if 'yuv' not in file_name:
                continue

            points = os.path.splitext(file_name)[0]
            damaged_stream = f'{DAMAGED_STREAMS}/{clip_name_qp}/iteration_{iteration}/{points}.h265'

            reconstructed_stream = f'{nn_dir_iter}/{points}.yuv'
            if not os.path.exists(reconstructed_stream):
                command = f'{T_APP_DECODER} -b {damaged_stream} -o {reconstructed_stream} -a 7'
                print(command)
                os.system(command)

            damaged_size = calc_bytes_size(damaged_stream)
            y_psnr_damaged, _, _, _, _, _ = calc_algo_PSNRs(source_clip_name, main_qp, points, iteration, 'nn')
            y_psnr_original = np.polyval(poly, damaged_size)
            print(points, damaged_size, y_psnr_damaged, y_psnr_original - y_psnr_damaged)

            point = Point(None, damaged_size, y_psnr_damaged, y_psnr_original - y_psnr_damaged)
            nn_points.append(point)

    nn_sizes = list(map(lambda p: p.bytes_size, nn_points))
    nn_psnrs = list(map(lambda p: p.PSNR, nn_points))
    xlims = [min(xlims[0], min(nn_sizes)), max(xlims[1], max(nn_sizes))]
    ylims = [min(ylims[0], min(nn_psnrs)), max(ylims[1], max(nn_psnrs))]

    if plot_mode == 'bitrate':
        nn_bitrates = list(map(lambda x: calc_bitrate(x), nn_sizes))
        plt.scatter(nn_bitrates, nn_psnrs, color='black', label=f'Segment NN, {max_iter - 1} iterations', s=1)
    elif plot_mode == 'size':
        plt.scatter(nn_sizes, nn_psnrs, color='black', label=f'Segment NN, {max_iter - 1}-{max_iter} iterations', s=1)

    if plot_mode == 'bitrate':
        plt.xlabel('Bitrate, kbps')
        plt.xlim([calc_bitrate(xlims[0]) - 1, calc_bitrate(xlims[1]) + 1])
    elif plot_mode == 'size':
        plt.xlabel('Size, bytes')
        plt.xlim([xlims[0] - 1, xlims[1] + 1])

    plt.ylim([min(np.polyval(poly, xlims[0]), ylims[0]) - 0.01, max(np.polyval(poly, xlims[1]), ylims[1]) + 0.01])
    plt.ylabel('Y-PSNR, dB')
    plt.grid()
    plt.legend()
    plt.savefig(f'{FIGURES_PATH}/{clip_name_qp}_{plot_mode}.png', format='png', dpi=500)
    plt.clf()


def create_rd_curve(source_clip_name, all_qps, title=None):
    original_sizes = []
    original_psnrs = []
    for qp in all_qps:
        original_size = calc_bytes_size(f'{ORIGINAL_STREAMS}/{source_clip_name}_QP_{qp}.h265')
        original_psnr, _, _, _, _, _ = calc_ideal_psnrs(source_clip_name, qp)
        original_sizes.append(original_size)
        original_psnrs.append(float(original_psnr))
        print(qp, calc_bitrate(original_size), original_psnr)

    deg = len(original_sizes) - 1
    poly = np.polyfit(original_sizes, original_psnrs, deg)

    cycle_colors = plt.rcParams['axes.prop_cycle'].by_key()['color']
    plt.title(title, fontweight='bold', color='#30302f')
    sizes_linspace = np.linspace(min(original_sizes), max(original_sizes), 500)
    psnrs_linspace = [np.polyval(poly, i) for i in sizes_linspace]
    bitrates_linspace = list(map(lambda x: calc_bitrate(x), sizes_linspace))
    plt.plot(bitrates_linspace, psnrs_linspace, color=cycle_colors[0], label='RD кривая')

    plt.plot(358.696, 33.476805, 'ro', label='Исходная точка', color='black')
    plt.plot(325.696, 33.400000, 'ro', label='"Хорошая" точка"', color='green')
    plt.plot(355.262, 33.166805, 'ro', label='"Плохая" точка', color='red')

    plt.ylim([32.8, 33.8])
    plt.xlabel('Bitrate, kbps')
    plt.ylabel('Y-PSNR, dB')
    # plt.grid()
    # plt.legend()
    plt.savefig(f'{FIGURES_PATH}/rd_curve.png', format='png', dpi=500)
    plt.clf()


if __name__ == '__main__':
    # downscale_video(input_yuv_path=f'{ONE_PKG_LOST}/touchdown_pass_1080p.yuv', input_resolution=(1080, 1920),
    #                 output_yuv_path=f'{ORIGINAL_YUVS}/touchdown_pass_640x384.yuv', output_resolution=(384, 640),
    #                 start_from=120, frames_number=24)
    # construct_nal_packages('tractor_640x384_QP_30')
    # construct_nal_packages('touchdown_pass_640x384_QP_40')
    # construct_nal_packages('shields_rotated_504x504_QP_30')
    # lose_bits('tractor_640x384_QP_30')
    # lose_bits('touchdown_pass_640x384_QP_40')
    # lose_bits('shields_rotated_504x504_QP_30')
    # decode('tractor_640x384_QP_30')
    # decode('touchdown_pass_640x384_QP_40')
    # decode('shields_rotated_504x504_QP_30')
    # calc_good_results('tractor_640x384', main_qp=30)
    # calc_good_results('touchdown_pass_640x384',
    #                   main_qp=30, all_qps=[28, 30, 32],
    #                   plot_mode='bitrate',
    #                   title='touchdown_pass, 640x384, QP = 30')
    # calc_good_results('touchdown_pass_640x384',
    #                   main_qp=35, all_qps=[33, 35, 37],
    #                   plot_mode='bitrate',
    #                   title='touchdown_pass, 640x384, QP = 35')
    # calc_good_results('touchdown_pass_640x384',
    #                   main_qp=40, all_qps=[38, 40, 42],
    #                   plot_mode='bitrate',
    #                   title='touchdown_pass, 640x384, QP = 40')
    create_rd_curve('touchdown_pass_640x384', all_qps=[32, 33, 35])
    x1, y1 = 358.696, 33.476805
    x2, y2 = 325.696, 33.400000
    x3 = (x1 + x2 - (y1 - y2) * math.sqrt(3)) / 2
    y3 = (y1 + y2 + (x2 - x1) * math.sqrt(3)) / 2
    print(x3, y3)
