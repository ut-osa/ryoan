import numpy as np
import os


def split_data(num_trainer=16):
    data = load_data()
    train_x = data['X_train']
    train_y = data['Y_train']
    n = len(train_x)
    size = n / num_trainer + 1  # data per trainer

    data_dir = './data/'
    if not os.path.exists(data_dir):
        os.mkdir(data_dir)

    # start training
    for i in range(num_trainer):
        x = train_x[size * i: size * (i + 1)]
        y = train_y[size * i: size * (i + 1)]
        fname = data_dir + 'cifar_trainer{}-{}.npz'.format(num_trainer, i)
        np.savez(fname, x, y)


def unpickle(file):
    try:
        import cPickle as pickle
        fo = open(file, 'rb')
        dict = pickle.load(fo)
        fo.close()
    except ImportError:
        import pickle
        fo = open(file, 'rb')
        dict = pickle.load(fo, encoding='latin1')
        fo.close()
    return dict


def load_data():
    xs = []
    ys = []
    for j in range(5):
        d = unpickle('../CIFAR/cifar-10-batches-py/data_batch_{}'.format(j + 1))
        x = d['data']
        y = d['labels']
        xs.append(x)
        ys.append(y)

    d = unpickle('../CIFAR/cifar-10-batches-py/test_batch')
    xs.append(d['data'])
    ys.append(d['labels'])

    x = np.concatenate(xs) / np.float32(255)
    y = np.concatenate(ys)
    x = np.dstack((x[:, :1024], x[:, 1024:2048], x[:, 2048:]))
    x = x.reshape((x.shape[0], 32, 32, 3)).transpose(0, 3, 1, 2)

    # subtract per-pixel mean
    pixel_mean = np.mean(x[0:50000], axis=0)
    # pickle.dump(pixel_mean, open("cifar10-pixel_mean.pkl","wb"))
    x -= pixel_mean

    # create mirrored images
    X_train = x[0:50000, :, :, :]
    Y_train = y[0:50000]

    # X_train_flip = X_train[:, :, :, ::-1]
    # Y_train_flip = Y_train
    # X_train = np.concatenate((X_train, X_train_flip), axis=0)
    # Y_train = np.concatenate((Y_train, Y_train_flip), axis=0)

    X_test = x[50000:, :, :, :]
    Y_test = y[50000:]
    return dict(
        X_train=X_train,
        Y_train=Y_train.astype('int32'),
        X_test=X_test,
        Y_test=Y_test.astype('int32'))


if __name__ == '__main__':
    split_data(1)
