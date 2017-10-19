from random import shuffle
import numpy as np
import glob
import tensorflow as tf
import cv2
import sys
import os

# 因为我装的是CPU版本的，运行起来会有'warning'，解决方法入下，眼不见为净~
os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'

shuffle_data = True
image_path = '/path/to/image/*.jpg'

# 取得该路径下所有图片的路径，type（addrs）= list
addrs = glob.glob(image_path)
# 标签数据的获得具体情况具体分析，type（labels）= list
labels = ...

# 这里是打乱数据的顺序
if shuffle_data:
    c = list(zip(addrs, labels))
    shuffle(c)
    addrs, labels = zip(*c)

# 按需分割数据集
train_addrs = addrs[0:int(0.7*len(addrs))]
train_labels = labels[0:int(0.7*len(labels))]

val_addrs = addrs[int(0.7*len(addrs)):int(0.9*len(addrs))]
val_labels = labels[int(0.7*len(labels)):int(0.9*len(labels))]

test_addrs = addrs[int(0.9*len(addrs)):]
test_labels = labels[int(0.9*len(labels)):]

# 上面不是获得了image的地址么，下面这个函数就是根据地址获取图片
def load_image(addr):  # A function to Load image
    img = cv2.imread(addr)
    img = cv2.resize(img, (224, 224), interpolation=cv2.INTER_CUBIC)
    img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
    # 这里/255是为了将像素值归一化到[0，1]
    img = img / 255.
    img = img.astype(np.float32)
    return img

# 将数据转化成对应的属性
def _int64_feature(value):  
    return tf.train.Feature(int64_list=tf.train.Int64List(value=[value]))


def _bytes_feature(value):
    return tf.train.Feature(bytes_list=tf.train.BytesList(value=[value]))


def _float_feature(value):
    return tf.train.Feature(float_list=tf.train.FloatList(value=[value]))

# 下面这段就开始把数据写入TFRecods文件

train_filename = '/path/to/train.tfrecords'  # 输出文件地址

# 创建一个writer来写 TFRecords 文件
writer = tf.python_io.TFRecordWriter(train_filename)

for i in range(len(train_addrs)):
    # 这是写入操作可视化处理
    if not i % 1000:
        print('Train data: {}/{}'.format(i, len(train_addrs)))
        sys.stdout.flush()
    # 加载图片
    img = load_image(train_addrs[i])

    label = train_labels[i]

    # 创建一个属性（feature）
    feature = {'train/label': _int64_feature(label),
               'train/image': _bytes_feature(tf.compat.as_bytes(img.tostring()))}

    # 创建一个 example protocol buffer
    example = tf.train.Example(features=tf.train.Features(feature=feature))

    # 将上面的example protocol buffer写入文件
    writer.write(example.SerializeToString())

writer.close()
sys.stdout.flush()