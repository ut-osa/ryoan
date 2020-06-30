import theano
import theano.tensor as T
import numpy as np
import time
import sys
from collections import OrderedDict

epochs = 10
use_ryoan = hasattr(theano, 'trusted_build_network')
if len(sys.argv) > 1:
    epochs = int(sys.argv[1])

np.random.seed(0)

def build_network(input_var, input_size, target_size):
    # weight for input to hidden layer
    W1 = theano.shared(value=np.random.normal(scale=1e-2, size=(input_size, 8)).astype(theano.config.floatX),
                       name='W1')
    # activation of hidden layer
    a1 = T.nnet.relu(T.dot(input_var, W1))
    # weight for hidden to output layer
    W2 = theano.shared(value=np.random.normal(scale=1e-2, size=(8, target_size)).astype(theano.config.floatX),
                       name='W2')

    # softmax on output layer
    output = T.nnet.softmax(T.dot(a1, W2))
    params = [W1, W2]
    return output, params


learning_rate = 1.
# generate fake data
x = np.random.normal(0, 1, size=(30, 10)).astype(theano.config.floatX)
y = np.zeros(shape=(30, )).astype('int32')
y[:10] = 1
y[10:20] = 2

# create placeholder for data
input_var = T.matrix('x')
target_var = T.ivector('y')

# define model, output is the probability vector
output, params = build_network(input_var, x.shape[1], len(np.unique(y)))

# loss function
loss = T.nnet.categorical_crossentropy(output, target_var).mean()

# calculate gradient
grads = T.grad(loss, params)

# define gradient update
updates = OrderedDict()
for param, grad in zip(params, grads):
	updates[param] = param - learning_rate * grad

# define function for training
# given input and target return loss and performs updates
if use_ryoan:
    theano.trusted_build_network([input_var, target_var], loss, updates=updates)
else:
    train_f = theano.function([input_var, target_var], loss, updates=updates,
                              no_default_updates=True)
#theano.printing.pydotprint(train_f, outfile="example_graph.png", var_with_name_simple=True)

start_time = time.time()
# actual training loop
cost = None
original_params = [param.get_value() for param in params]
if use_ryoan:
    theano.trusted_train(x, y, epochs)
else:
    for epoch in range(epochs):
        cost = train_f(x, y)
        print( "at epoch {}, loss {}".format(epochs - 1, cost) )
print("--- %s seconds ---" % (time.time() - start_time))
