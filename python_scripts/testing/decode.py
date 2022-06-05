import os

from testing.settings.common import T_APP_DECODER, ERROR_ALGO_PARAMS
from testing.settings.default import RECONSTRUCTED_STREAMS, DAMAGED_STREAMS

DECODING_INFO = {
    'Johnny_hd': [2, 3, 4, 5, 6],
    'KristenAndSara_hd': [2, 3, 4, 5, 6],
    'flower_cif': [2, 3, 4, 5, 6],
    'grandma_qcif': [2, 3, 4, 5, 6],
    'life_hd': [2, 3, 4, 5, 6],
    'pamphlet_cif': [2, 3, 4, 5, 6],
    'pedestrian_area_hd': [2, 3, 4, 5, 6],
    'suzie_qcif': [2, 3, 4, 5, 6],
    'touchdown_pass_hd': [2, 3, 4, 5, 6],
    'tractor_hd': [2, 3, 4, 5, 6],
}

PROBS = [0.02, 0.04, 0.06, 0.08, 0.10]


def decode():
    for prob in PROBS:
        for clip_name in DECODING_INFO.keys():
            recon_folder = f'{RECONSTRUCTED_STREAMS}/{prob}'
            if not os.path.exists(recon_folder):
                os.makedirs(recon_folder)
            for algo in DECODING_INFO[clip_name]:
                params = ERROR_ALGO_PARAMS[algo]
                print(prob, clip_name, algo, params)
                damaged_stream = f'{DAMAGED_STREAMS}/{prob}/{clip_name}.h265'
                reconstructed_stream = f'{recon_folder}/{clip_name}_{algo}.yuv'
                command = f'{T_APP_DECODER} -b {damaged_stream} -o {reconstructed_stream} -a {algo} {params}'
                print(command)
                os.system(f'{command}')


if __name__ == '__main__':
    decode()
