import os
import subprocess

import matplotlib.pyplot as plt
from matplotlib import gridspec

from testing.settings.common import RESOLUTION, DISPLAY_NAMES
from testing.settings.default import ORIGINAL_STREAMS, ORIGINAL_YUVS, RECONSTRUCTED_STREAMS, FIGURES_PATH, MAIN_QP

CLIPS = [
    'Johnny_hd',
    'KristenAndSara_hd',
    'flower_cif',
    'grandma_qcif',
    'life_hd',
    'pamphlet_cif',
    'pedestrian_area_hd',
    'suzie_qcif',
    'touchdown_pass_hd',
    'tractor_hd',
]


def get_original_streams_yuvs():
    for clip_name in CLIPS:
        original_stream = f'{ORIGINAL_STREAMS}/{clip_name}.h265'
        original_yuv = f'{ORIGINAL_STREAMS}/{clip_name}.yuv'
        command = f'ffmpeg -y -i {original_stream} {original_yuv}'
        # print(command)
        os.system(command)


def calc_ideal_metric(clip_name, metric):
    width, height = RESOLUTION[clip_name]
    yuv_desc = f'-s:v {width}x{height} -c:v rawvideo -pix_fmt yuv420p'
    command = f'ffmpeg ' \
              f'{yuv_desc} -i {ORIGINAL_YUVS}/{clip_name}.yuv ' \
              f'-i {ORIGINAL_STREAMS}/{clip_name}.h265 ' \
              f'-lavfi {metric} -f null -'
    # print(command)
    process = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return process.stderr.splitlines()[-1].split()[4:]


def calc_algo_metric(clip_name, prob, error_algo, metric):
    width, height = RESOLUTION[clip_name]
    yuv_desc = f'-s:v {width}x{height} -c:v rawvideo -pix_fmt yuv420p'
    command = f'ffmpeg ' \
              f'{yuv_desc} -i {ORIGINAL_YUVS}/{clip_name}.yuv ' \
              f'{yuv_desc} -i {RECONSTRUCTED_STREAMS}/{prob}/{clip_name}_{error_algo}.yuv ' \
              f'-lavfi {metric} -f null -'
    # print(command)
    process = subprocess.run(command, shell=True, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    return process.stderr.splitlines()[-1].split()[4:]


def split_into_PSNRs(metric_output):
    psnrs = list(map(lambda x: x.decode('utf-8').split(':')[1], metric_output))
    y_psnr, u_psnr, v_psnr, avg_psnr, min_psnr, max_psnr = psnrs
    return y_psnr, u_psnr, v_psnr, avg_psnr, min_psnr, max_psnr


def split_into_SSIMs(metric_output):
    y_ssim = metric_output[0].decode('utf-8').split(':')[1]
    u_ssim = metric_output[2].decode('utf-8').split(':')[1]
    v_ssim = metric_output[4].decode('utf-8').split(':')[1]
    all_ssim = metric_output[6].decode('utf-8').split(':')[1]
    return y_ssim, u_ssim, v_ssim, all_ssim


def calc_ideal_PSNRs(clip_name):
    metric_output = calc_ideal_metric(clip_name, 'psnr')
    return split_into_PSNRs(metric_output)


def calc_algo_PSNRs(clip_name, prob, error_algo):
    metric_output = calc_algo_metric(clip_name, prob, error_algo, 'psnr')
    return split_into_PSNRs(metric_output)


def calc_ideal_SSIMs(clip_name):
    metric_output = calc_ideal_metric(clip_name, 'ssim')
    return split_into_SSIMs(metric_output)


def calc_algo_SSIMs(clip_name, prob, error_algo):
    metric_output = calc_algo_metric(clip_name, prob, error_algo, 'ssim')
    return split_into_SSIMs(metric_output)


ERROR_ALGORITHMS = {
    'Johnny_hd': [2, 3, 4, 6],
    'KristenAndSara_hd': [2, 3, 4, 6, 7],
    'flower_cif': [2, 3, 4, 6, 7],
    # 'flower_cif': [2, 5],
    'grandma_qcif': [2, 3, 4, 6, 7],
    'life_hd': [2, 3, 4, 6, 7],
    'pamphlet_cif': [2, 3, 4, 6, 7],
    'pedestrian_area_hd': [2, 3, 4, 6],
    # 'pedestrian_area_hd': [2, 5],
    'suzie_qcif': [2, 3, 4, 6, 7],
    # 'suzie_qcif': [2, 5],
    'touchdown_pass_hd': [2, 3, 4, 6, 7],
    # 'touchdown_pass_hd': [2, 5],
    # 'tractor_hd': [2, 3, 4, 6, 7],
    'tractor_hd': [2, 3, 4, 6, 7]
}

ALGORITHM_NAMES = {
    2: 'Frame Copy',
    3: 'Motion Predict',
    4: 'Frame Stitching',
    5: 'Frame NN',
    6: 'Segment NN',
    7: 'Segment NN + Post-Processing'
}

# PROBS = [0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.14, 0.16, 0.18, 0.20]


# PROBS = [0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.14]
# PROBS = [0.02, 0.04, 0.06, 0.08, 0.10, 0.12]
PROBS = [0.02, 0.04, 0.06, 0.08]


def draw_PSNR_plots():
    for clip_name in CLIPS:
        Y_PSNR_ideal, U_PSNR_ideal, V_PSNR_ideal, avg_PSNR_ideal, _, _ = calc_ideal_PSNRs(clip_name)
        plt.title(clip_name)
        for algo in ERROR_ALGORITHMS[clip_name]:
            Y_PSNRs = [float(Y_PSNR_ideal)]
            percents = [0]
            for prob in PROBS:
                if algo == 7:
                    Y_PSNR, U_PSNR, V_PSNR, avg_PSNR, _, _ = calc_algo_PSNRs(clip_name, prob, '6_reversed')
                else:
                    Y_PSNR, U_PSNR, V_PSNR, avg_PSNR, _, _ = calc_algo_PSNRs(clip_name, prob, algo)
                percents.append(prob * 100)
                Y_PSNRs.append(float(Y_PSNR))
            print(clip_name, algo, percents, Y_PSNRs)
            plt.xticks(percents)
            plt.plot(percents, Y_PSNRs, label=ALGORITHM_NAMES[algo])
        plt.xlabel('Percentage of errors')
        plt.ylabel('Y-PSNR')
        plt.legend()
        plt.savefig(f'{FIGURES_PATH}/{clip_name}_{MAIN_QP}_PSNR.png', format='png', dpi=90)
        plt.clf()


def draw_SSIM_plots():
    for clip_name in CLIPS:
        _, _, _, all_ssim_ideal = calc_ideal_SSIMs(clip_name)
        plt.title(clip_name)
        for algo in ERROR_ALGORITHMS[clip_name]:
            all_SSIMs = []
            percents = []
            for prob in PROBS:
                _, _, _, all_ssim = calc_algo_SSIMs(clip_name, prob, algo)
                percents.append(prob * 100)
                all_SSIMs.append(float(all_ssim))
            print(clip_name, algo, percents, all_SSIMs)
            plt.xticks(percents)
            plt.plot(percents, all_SSIMs, label=ALGORITHM_NAMES[algo])
        plt.xlabel('Percentage of errors')
        plt.ylabel('All-SSIM')
        plt.legend()
        plt.savefig(f'{FIGURES_PATH}/{clip_name}_All_SSIM.png', format='png', dpi=90)
        plt.clf()


def calc_PSNR_results():
    print('clip_name,y_psnr_ideal,avg_psnr_ideal,prob,algo,y_psnr_algo,avg_psnr_algo')
    for clip_name in CLIPS:
        Y_PSNR_ideal, U_PSNR_ideal, V_PSNR_ideal, avg_PSNR_ideal, _, _ = calc_ideal_PSNRs(clip_name)
        print(f'{clip_name},0,-,{Y_PSNR_ideal}')
        for prob in PROBS:
            for algo in ERROR_ALGORITHMS[clip_name]:
                Y_PSNR, U_PSNR, V_PSNR, avg_PSNR, _, _ = calc_algo_PSNRs(clip_name, prob, algo)
                print(f'{clip_name},{Y_PSNR_ideal},{avg_PSNR_ideal},{prob},{ALGORITHM_NAMES[algo]},{Y_PSNR},{avg_PSNR}')


def calc_SSIM_results():
    print('clip_name,all_ssim_ideal,prob,algo,all_ssim_algo')
    for clip_name in CLIPS:
        Y_SSIM_ideal, U_SSIM_ideal, V_SSIM_ideal, all_SSIM_ideal = calc_ideal_SSIMs(clip_name)
        for prob in PROBS:
            for algo in ERROR_ALGORITHMS[clip_name]:
                Y_SSIM, U_SSIM, V_SSIM, all_SSIM = calc_algo_SSIMs(clip_name, prob, algo)
                print(f'{clip_name},{all_SSIM_ideal},{prob},{ALGORITHM_NAMES[algo]},{all_SSIM}')


def draw_nn_plot(clips, error_algorithms, probs, columns, rows, output_name, figsize):
    assert len(clips) == columns * rows

    print('Тестовое видео', end=' & ')
    print('Алгоритм', end=' & ')
    for i in range(len(probs)):
        end_symbol = ' & ' if i != len(probs) - 1 else ' \\\\\\hline\n'
        print(int(probs[i] * 100), end=end_symbol)

    fig = plt.figure(figsize=figsize)
    gs = gridspec.GridSpec(rows, columns)
    # gs.update(wspace=0.25, hspace=0.35)
    gs.update(wspace=0.25, hspace=0.25)

    gains_by_algo = {i: 0 for i in error_algorithms}

    for i in range(1, columns * rows + 1):
        clip_name = clips[i - 1]
        Y_PSNR_ideal, U_PSNR_ideal, V_PSNR_ideal, avg_PSNR_ideal, _, _ = calc_ideal_PSNRs(clip_name)

        ax = fig.add_subplot(gs[i - 1])
        ax.set_title(f'{DISPLAY_NAMES[clip_name]}', fontsize=12, fontweight='bold', color='#30302f', loc='center')
        ax.grid()

        PSNR_by_algo = {}
        for algo in error_algorithms:
            Y_PSNRs = [float(Y_PSNR_ideal)]
            percents = [0]
            for prob in probs:
                if algo == 7:
                    Y_PSNR, U_PSNR, V_PSNR, avg_PSNR, _, _ = calc_algo_PSNRs(clip_name, prob, '6_reversed')
                else:
                    Y_PSNR, U_PSNR, V_PSNR, avg_PSNR, _, _ = calc_algo_PSNRs(clip_name, prob, algo)
                percents.append(prob * 100)
                Y_PSNRs.append(float(Y_PSNR))
                PSNR_by_algo[(algo, prob)] = float(Y_PSNR)
                if algo == 5:
                    gains_by_algo[5] += PSNR_by_algo[(5, prob)] - PSNR_by_algo[(2, prob)]
                if algo == 6:
                    gains_by_algo[6] += PSNR_by_algo[(6, prob)] - PSNR_by_algo[(2, prob)]
                if algo == 7:
                    gains_by_algo[7] += PSNR_by_algo[(7, prob)] - PSNR_by_algo[(2, prob)]
            print(DISPLAY_NAMES[clip_name], end=' & ')
            print(ALGORITHM_NAMES[algo], end=' & ')
            for k in range(len(Y_PSNRs)):
                end_symbol = ' & ' if k != len(Y_PSNRs) - 1 else ' \\\\\\hline\n'
                print(round(Y_PSNRs[k], 3), end=end_symbol)
            plt.xticks(percents)
            plt.plot(percents, Y_PSNRs, label=ALGORITHM_NAMES[algo])
        plt.xlabel('Packet loss rate, %')
        plt.ylabel('Y-PSNR, dB')
        plt.legend(fontsize=9)

    if 5 in gains_by_algo:
        print(f'Frame NN average gain: {gains_by_algo[5] / len(clips) / len(probs)}')
    if 6 in gains_by_algo:
        print(f'Segment NN average gain: {gains_by_algo[6] / len(clips) / len(probs)}')
    if 7 in gains_by_algo:
        print(f'Segment NN + Post-Processing average gain: {gains_by_algo[7] / len(clips) / len(probs)}')

    plt.savefig(f'{FIGURES_PATH}/{output_name}.pdf', bbox_inches='tight', pad_inches=0.07)
    os.system(f'pdftoppm -png -r 300 {FIGURES_PATH}/{output_name}.pdf {FIGURES_PATH}/{output_name}')
    plt.clf()


def draw_base_nn_plot():
    return draw_nn_plot(clips=['flower_cif', 'pedestrian_area_hd', 'touchdown_pass_hd', 'tractor_hd'],
                        error_algorithms=[2, 3, 4, 5],
                        probs=[0.02, 0.04, 0.06, 0.08, 0.10, 0.12, 0.14, 0.16, 0.18, 0.20],
                        columns=2,
                        rows=2,
                        output_name='base-nn',
                        figsize=(8, 7))


def draw_advanced_nn_plot():
    return draw_nn_plot(
        clips=['flower_cif', 'pedestrian_area_hd', 'touchdown_pass_hd', 'tractor_hd', 'suzie_qcif', 'life_hd'],
        error_algorithms=[2, 3, 4, 6, 7],
        probs=[0.02, 0.04, 0.06, 0.08, 0.10],
        columns=2,
        rows=3,
        output_name='advanced-nn',
        figsize=(10, 13))


def draw_advanced_nn_diff():
    probs = [0.02, 0.04, 0.06, 0.08, 0.10]
    clips = [
        'flower_cif',
        'pedestrian_area_hd',
        'touchdown_pass_hd',
        'tractor_hd',
        'suzie_qcif',
        'life_hd'
    ]

    for clip_name in clips:
        deltas_Y_PSNR = [0.0]
        percents = [0]
        for prob in probs:
            Y_PSNR_better, _, _, _, _, _ = calc_algo_PSNRs(clip_name, prob, '6_reversed')
            Y_PSNR_worse, _, _, _, _, _ = calc_algo_PSNRs(clip_name, prob, '6')
            percents.append(prob * 100)
            deltas_Y_PSNR.append(float(Y_PSNR_better) - float(Y_PSNR_worse))
        print(clip_name, percents, deltas_Y_PSNR)
        plt.xticks(percents)

        # cubic_interploation_model = interp1d(percents, deltas_Y_PSNR, kind="cubic")
        # X_ = np.linspace(min(percents), max(percents), 500)
        # Y_ = cubic_interploation_model(X_)
        # plt.plot(X_, Y_, label=display_names[clip_name])

        plt.plot(percents, deltas_Y_PSNR, label=DISPLAY_NAMES[clip_name])
    plt.xlabel('Percentage of errors')
    plt.ylabel('delta-Y-PSNR')
    plt.legend()

    plt.savefig(f'{FIGURES_PATH}/advanced-diff-nn.pdf', bbox_inches='tight', pad_inches=0.07)
    os.system(f'pdftoppm -png -r 300 {FIGURES_PATH}/advanced-diff-nn.pdf {FIGURES_PATH}/advanced-diff-nn')
    plt.clf()


if __name__ == '__main__':
    if not os.path.exists(FIGURES_PATH):
        os.makedirs(FIGURES_PATH)
    # get_original_yuvs()
    # calc_PSNR_results()
    # calc_SSIM_results()
    # draw_PSNR_plots()
    # draw_base_nn_plot()
    draw_advanced_nn_plot()
    # draw_advanced_nn_diff()
    # draw_SSIM_plots()
