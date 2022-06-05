import os

from testing.settings.common import DISPLAY_NAMES
from testing.settings.default import ORIGINAL_YUVS, RECONSTRUCTED_STREAMS, DAMAGED_STREAMS

CLIPS = [
    'flower_cif',
    'pedestrian_area_hd',
    'touchdown_pass_hd',
    'tractor_hd',
    'suzie_qcif',
    'life_hd'
]
PROBS = [
    0.02,
    0.04,
    0.06,
    0.08,
    0.10
]


def get_clip_damaged_frames_number(clip_name, prob):
    stats_file_name = f'{DAMAGED_STREAMS}/{prob}/{clip_name}.txt'
    with open(stats_file_name, 'r') as f:
        lines_all = list(map(lambda line: list(map(int, (line.split(' ')))), f.readlines()))
        damaged_frames_all = sorted(list(set(map(lambda line: line[0], lines_all))))
        return len(damaged_frames_all)


def get_psnr_command(clip_name, prob, error_algo, stats_file_name):
    return f'ffmpeg ' \
           f'-s:v 160x128 -c:v rawvideo -pix_fmt yuv420p -i {ORIGINAL_YUVS}/{clip_name}.yuv ' \
           f'-s:v 160x128 -c:v rawvideo -pix_fmt yuv420p -i {RECONSTRUCTED_STREAMS}/{prob}/{clip_name}_{error_algo}.yuv ' \
           f'-lavfi psnr="stats_file={stats_file_name}" -f null - 2>/dev/null'


def main():
    became_better_percent_sum = [0 for _ in PROBS]
    changed_percent_sums = [0 for _ in PROBS]
    psnr_gain_sums = [0 for _ in PROBS]

    for clip_name in CLIPS:
        for i in range(len(PROBS)):
            prob = PROBS[i]
            command_1 = get_psnr_command(clip_name, prob, '6', 'stats_1.log')
            command_2 = get_psnr_command(clip_name, prob, '6_reversed', 'stats_2.log')
            os.system(command_1)
            os.system(command_2)
            with open('stats_1.log') as f_1:
                lines_1 = list(map(lambda line: line.split(' ')[6].split(':')[1], f_1.readlines()))
            with open('stats_2.log') as f_2:
                lines_2 = list(map(lambda line: line.split(' ')[6].split(':')[1], f_2.readlines()))
            assert len(lines_1) == len(lines_2)
            became_better = 0
            became_worse = 0
            diff = 0
            for psnr_1, psnr_2 in zip(lines_1, lines_2):
                if psnr_1 != psnr_2:
                    psnr_1 = float(psnr_1)
                    psnr_2 = float(psnr_2)
                    diff += psnr_2 - psnr_1
                    if psnr_2 > psnr_1:
                        became_better += 1
                    else:
                        became_worse += 1
            damaged_frames_number = get_clip_damaged_frames_number(clip_name, prob)
            became_better_percent = 100 * became_better / (became_better + became_worse)
            changed_percent = 100 * (became_better + became_worse) / damaged_frames_number
            psnr_gain = diff / (became_better + became_worse)
            start = '& ' if prob != PROBS[0] else '\multirow{5}{*}{' + DISPLAY_NAMES[clip_name] + '} &'
            color = '{\\cellcolor{my-green}' if psnr_gain >= 0 else '{\\cellcolor{my-red}'
            end = '\\\\\\cline{2-5}\n' if prob != PROBS[-1] else '\\\\\\thickhline\n'
            print(f'{start}'
                  f'{int(prob * 100)} & '
                  f'{int(changed_percent)} & '
                  f'{int(became_better_percent)} & '
                  f'{color}{round(psnr_gain, 3)}' + '}',
                  end=end)
            became_better_percent_sum[i] += became_better_percent
            changed_percent_sums[i] += changed_percent
            psnr_gain_sums[i] += psnr_gain
    for i in range(len(PROBS)):
        prob = PROBS[i]
        became_better_percent = int(became_better_percent_sum[i] / len(CLIPS))
        psnr_gain = psnr_gain_sums[i] / len(CLIPS)
        changed_percent = int(changed_percent_sums[i] / len(CLIPS))
        start = '& ' if i != 0 else '\multirow{5}{*}{\\textbf{Среднее}} &'
        color = '{\\cellcolor{my-green}' if psnr_gain >= 0 else '{\\cellcolor{my-red}'
        end = '\\\\\\cline{2-5}\n' if i != len(PROBS) - 1 else '\\\\\\thickhline\n'
        print(f'{start}'
              '\\textbf{' + f'{int(prob * 100)}' + '} &'
              '\\textbf{' + f'{changed_percent}' + '} &'
              '\\textbf{' + f'{became_better_percent}' + '} &'
              '\\textbf{' + f'{color}{round(psnr_gain, 3)}' + '}}',
              end=end)
    print(f'Overall average: '
          f'{sum(changed_percent_sums) / len(CLIPS) / len(PROBS)} '
          f'{sum(became_better_percent_sum) / len(CLIPS) / len(PROBS)} '
          f'{sum(psnr_gain_sums) / len(CLIPS) / len(PROBS)}')


if __name__ == '__main__':
    main()
