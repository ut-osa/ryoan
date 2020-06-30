import numpy as np

MEAN = np.asarray([0.48352443, 0.44251362, 0.38846879], dtype=np.float32)
STD = np.asarray([0.27608174, 0.26501602, 0.27680763], dtype=np.float32)


def load_batch_path_label(batch_size=64):
    filename = '../IMAGENET/imagenet_subset_batch{}_path.npz'.format(batch_size)
    with np.load(filename) as f:
        batch_paths, test_p, test_y = f['arr_0'], f['arr_1'], f['arr_2']
    print(len(batch_paths) / 8 * 64)
    return batch_paths, test_p, test_y.astype(np.int32)


def horizontal_flip(img, p=0.5):
    p_ = np.random.random()
    if p_ <= p:
        return img[:, ::-1]
    return img


def color_normalization(img, mean=MEAN, std=STD):
    for i in range(3):
        img[..., i] = (img[..., i] - mean[i]) / std[i]
    return img


def load_batch_data(path):
    try:
        with np.load(path.decode('utf-8')) as f:
            imgs, y = f['arr_0'], f['arr_1']
    except OSError:
        return None, None

    batch_size = len(imgs)

    x = np.zeros((batch_size, 3, 112, 112), dtype=np.float32)
    crops = np.random.random_integers(0, high=16, size=(batch_size, 2))
    for i, img in enumerate(imgs):
        img = color_normalization(img / np.float32(255.0))
        img = horizontal_flip(img)
        img = img[crops[i, 0]: (crops[i, 0] + 112), crops[i, 1]: (crops[i, 1] + 112)]
        x[i] = img.transpose(2, 0, 1).astype(np.float32)

    return x, y.astype(np.int32)


if __name__ == '__main__':
    load_batch_path_label()