import sys

sys.path.append('/Users/lizarasho/itmo/diploma/python_scripts')

import argparse
import os
import cv2
import numpy as np

from testing.settings.common import DIPLOMA_FOLDER
from utils.common import calc_bytes_size, calc_frame_size
from utils.yuv_utils import VideoCaptureYUV

IS_REVERSED = False
SHOW_VIDEOS = False

WINDOW_W = 160
WINDOW_H = 128

suffix = '_reversed' if IS_REVERSED else ''

PREDNET_FOLDER = DIPLOMA_FOLDER + '/python_scripts/prednet'

TEST_SOURCES = f'{PREDNET_FOLDER}/dataset/tmp_test.hkl'
TEST_FILE = f'{PREDNET_FOLDER}/dataset/tmp_source.hkl'

WEIGHTS_FILE = f'{PREDNET_FOLDER}/models/prednet_weights{suffix}.hdf5'
JSON_FILE = f'{PREDNET_FOLDER}/models/prednet_model{suffix}.json'

RESULTS_FOLDER = f'{PREDNET_FOLDER}/results{suffix}'

RESULT_FRAME_PATH = f'{PREDNET_FOLDER}/temporary_frame.yuv'

N_PLOT = 40
BATCH_SIZE = 10
NT = 10


def get_clip_name(full_clip_path):
    base = os.path.basename(full_clip_path)
    return os.path.splitext(base)[0]


def calc_rectangle_position(full_w, full_h, x1, y1, x2, y2):
    slice_w = np.abs(x2 - x1)
    slice_h = np.abs(y2 - y1)
    margin_w = (WINDOW_W - slice_w) // 2
    margin_h = (WINDOW_H - slice_h) // 2
    X1 = x1 - margin_w
    X2 = x2 + margin_w
    Y1 = y1 - margin_h
    Y2 = y2 + margin_h
    if X1 < 0:
        X2 += -X1
        X1 += -X1
    if Y1 < 0:
        Y2 += -Y1
        Y1 += -Y1
    if X2 > full_w:
        X1 -= X2 - full_w
        X2 -= X2 - full_w
    if Y2 > full_h:
        Y1 -= Y2 - full_h
        Y2 -= Y2 - full_h
    return X1, Y1


def prepare_test_data(test_yuv_path, full_width, full_height, x1, y1, x2, y2):
    import hickle as hkl

    bytes_size = calc_bytes_size(test_yuv_path)
    frame_size = calc_frame_size(full_width, full_height)
    frames_number = bytes_size // frame_size
    print(f'Current number of frames: {frames_number}')

    cap = VideoCaptureYUV(test_yuv_path, full_width, full_height)

    if frames_number >= 9:
        cap.skip_frames(frames_number - 9)
        current_frame = frames_number - 9
    else:
        current_frame = 0

    X1, Y1 = calc_rectangle_position(full_width, full_height, x1, y1, x2, y2)
    X = []
    source_list = []
    while current_frame < frames_number:
        ret, yuv_frame = cap.read()
        if ret:
            yuv_frame = yuv_frame.reshape(cap.shape)
            bgr_frame = cv2.cvtColor(yuv_frame, cv2.COLOR_YUV2BGR_I420)
            assert bgr_frame.shape == (full_height, full_width, 3)

            crop_bgr_frame = bgr_frame[Y1:(Y1 + WINDOW_H), X1:(X1 + WINDOW_W)]
            # cv2.imshow("Cropped", crop_bgr_frame)
            # cv2.waitKey(0)
            if (current_frame == 0) and (frames_number < 9):
                for _ in range(9 - frames_number):
                    X.append(crop_bgr_frame)
                    source_list.append(test_yuv_path)
            X.append(crop_bgr_frame)
            source_list.append(test_yuv_path)

            current_frame += 1
        else:
            break
    X.append(np.empty(shape=(WINDOW_H, WINDOW_W, 3)))
    source_list.append(test_yuv_path)

    X = np.array(X)
    source_list = np.array(source_list)

    assert len(X) == 10
    assert len(source_list) == 10
    assert current_frame == frames_number

    hkl.dump(X, TEST_FILE)
    hkl.dump(source_list, TEST_SOURCES)

    return current_frame, X1, Y1


def evaluate():
    from keras import Input
    from keras.engine import Model
    from keras.models import model_from_json
    from prednet.model import PredNet
    from prednet.data_utils import SequenceGenerator
    # from prednet import PredNet
    # from data_utils import SequenceGenerator

    # Load trained model
    f = open(JSON_FILE, 'r')
    json_string = f.read()
    f.close()
    train_model = model_from_json(json_string, custom_objects={'PredNet': PredNet})
    train_model.load_weights(WEIGHTS_FILE)

    # Create testing model (to output predictions)
    layer_config = train_model.layers[1].get_config()
    layer_config['output_mode'] = 'prediction'
    data_format = layer_config['data_format'] if 'data_format' in layer_config else layer_config['dim_ordering']
    test_prednet = PredNet(weights=train_model.layers[1].get_weights(), **layer_config)
    input_shape = list(train_model.layers[0].batch_input_shape[1:])
    input_shape[0] = NT
    inputs = Input(shape=tuple(input_shape))
    predictions = test_prednet(inputs)
    test_model = Model(inputs=inputs, outputs=predictions)

    test_generator = SequenceGenerator(TEST_FILE, TEST_SOURCES, NT,
                                       sequence_start_mode='unique',
                                       data_format=data_format)
    X_test = test_generator.create_all()
    X_hat = test_model.predict(X_test, BATCH_SIZE)
    if data_format == 'channels_first':
        X_test = np.transpose(X_test, (0, 1, 3, 4, 2))
        X_hat = np.transpose(X_hat, (0, 1, 3, 4, 2))

    os.remove(TEST_FILE)
    os.remove(TEST_SOURCES)

    return X_test, X_hat


def get_results(X_test, X_hat, test_yuv_path, lost_frame_number):
    from PIL import Image
    from matplotlib import pyplot as plt, gridspec

    # Compare MSE of PredNet predictions vs. using last frame.  Write results to prediction_scores.txt
    mse_model = np.mean((X_test[:, 1:] - X_hat[:, 1:]) ** 2)  # look at all timesteps except the first
    mse_prev = np.mean((X_test[:, :-1] - X_test[:, 1:]) ** 2)

    save_dir = os.path.join(RESULTS_FOLDER, f'{get_clip_name(test_yuv_path)}', f'{lost_frame_number}')

    if not os.path.exists(save_dir):
        os.makedirs(save_dir)

    with open(f'{save_dir}/prediction_scores.txt', 'w') as f:
        f.write("Model MSE: %f\n" % mse_model)
        f.write("Previous Frame MSE: %f" % mse_prev)

    # Plot some predictions
    aspect_ratio = float(X_hat.shape[2]) / X_hat.shape[3]
    plt.figure(figsize=(NT, 2 * aspect_ratio))
    gs = gridspec.GridSpec(2, NT)
    gs.update(wspace=0., hspace=0.)

    for t in range(NT):
        plt.subplot(gs[t])
        rgb_image_actual = cv2.cvtColor((X_test[0, t] * 255.).astype(np.uint8), cv2.COLOR_BGR2RGB)
        plt.imshow(rgb_image_actual, interpolation='none')
        plt.tick_params(axis='both', which='both', bottom='off', top='off', left='off', right='off',
                        labelbottom='off', labelleft='off')
        if t == 0:
            plt.ylabel('Actual', fontsize=10)
        if t < NT - 1:
            Image.fromarray(rgb_image_actual, 'RGB').save(f'{save_dir}/{t}_actual.png')

        plt.subplot(gs[t + NT])
        rgb_image_predicted = cv2.cvtColor((X_hat[0, t] * 255.).astype(np.uint8), cv2.COLOR_BGR2RGB)
        plt.imshow(rgb_image_predicted, interpolation='none')
        plt.tick_params(axis='both', which='both', bottom='off', top='off', left='off', right='off',
                        labelbottom='off', labelleft='off')
        if t == 0:
            plt.ylabel('Predicted', fontsize=10)
        if t == NT - 1:
            Image.fromarray(rgb_image_predicted, 'RGB').save(f'{save_dir}/{t}_predicted.png')

    plt.savefig(f'{save_dir}/{lost_frame_number}.png')
    plt.clf()


def write_frame(X_hat, x1, y1, x2, y2, X1, Y1):
    predicted_bgr_frame = (X_hat[0][-1] * 255).astype(np.uint8)
    dX = x1 - X1
    dY = y1 - Y1
    slice_w = np.abs(x2 - x1)
    slice_h = np.abs(y2 - y1)
    predicted_bgr_slice = predicted_bgr_frame[dY:(dY + slice_h), dX:(dX + slice_w)]

    predicted_yuv_slice = cv2.cvtColor(predicted_bgr_slice, cv2.COLOR_BGR2YUV_I420)
    with open(RESULT_FRAME_PATH, 'wb') as f:
        f.write(np.array(predicted_yuv_slice).flatten().tobytes())

    assert calc_frame_size(slice_w, slice_h) == calc_bytes_size(RESULT_FRAME_PATH)


def main():
    parser = argparse.ArgumentParser()

    parser.add_argument('-i', '--input_file', type=str, required=True, dest='input_file')
    parser.add_argument('--width', type=int, required=True, dest='width')
    parser.add_argument('--height', type=int, required=True, dest='height')
    parser.add_argument('--x1', type=int, required=True, dest='x1')
    parser.add_argument('--x2', type=int, required=True, dest='x2')
    parser.add_argument('--y1', type=int, required=True, dest='y1')
    parser.add_argument('--y2', type=int, required=True, dest='y2')

    args = parser.parse_args()
    test_yuv_path = args.input_file
    full_width = args.width
    full_height = args.height
    x1, y1 = args.x1, args.y1
    x2, y2 = args.x2, args.y2

    lost_frame_number, X1, Y1 = prepare_test_data(test_yuv_path, full_width, full_height, x1, y1, x2, y2)
    X_test, X_hat = evaluate()
    get_results(X_test, X_hat, test_yuv_path, lost_frame_number)
    write_frame(X_hat, x1, y1, x2, y2, X1, Y1)

    print(f'Frame {lost_frame_number} was successfully prepared ({test_yuv_path})')


if __name__ == '__main__':
    # prepare_test_data('/Users/lizarasho/itmo/diploma/lose_packages/input/yuvs/shields_rotated_504x504.yuv',
    #                   504, 504,
    #                   440, 440, 504, 504)
    # print(calc_rectangle_position(504, 504, 252, 252, 316, 316))
    main()
