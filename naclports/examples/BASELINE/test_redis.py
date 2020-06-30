import redis
import array

arr = array.array('f', [1., 2., 3., 4., 5., 6., 7., 8.])

r = redis.StrictRedis(host='localhost', port=6379, db=0)

reply = r.set('foo', arr.tostring())
r.execute_command('ADD', 'foo', arr.tostring())
string = r.get('foo')
arr = array.array('f')
arr.fromstring(string)
print arr
