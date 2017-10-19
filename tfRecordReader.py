import tensorflow as tf
import numpy as np
import matplotlib.pyplot as plt
import os

os.environ['TF_CPP_MIN_LOG_LEVEL'] = '2'
data_path = 'train.tfrecords'  # tfrecords 文件的地址


with tf.Session() as sess:
    # 先定义feature，这里要和之前创建的时候保持一致
    feature = {
        'train/image': tf.FixedLenFeature([], tf.string),
        'train/label': tf.FixedLenFeature([], tf.int64)
    }
    # 创建一个队列来维护输入文件列表
    filename_queue = tf.train.string_input_producer([data_path], num_epochs=1)

    # 定义一个 reader ，读取下一个 record
    reader = tf.TFRecordReader()
    _, serialized_example = reader.read(filename_queue)

    # 解析读入的一个record
    features = tf.parse_single_example(serialized_example, features=feature)

    # 将字符串解析成图像对应的像素组
    image = tf.decode_raw(features['train/image'], tf.float32)

    # 将标签转化成int32
    label = tf.cast(features['train/label'], tf.int32)

    # 这里将图片还原成原来的维度
    image = tf.reshape(image, [224, 224, 3])

    # 你还可以进行其他一些预处理....

    # 这里是创建顺序随机 batches(函数不懂的自行百度)
    images, labels = tf.train.shuffle_batch([image, label], batch_size=10, capacity=30, min_after_dequeue=10)

    # 初始化
    init_op = tf.group(tf.global_variables_initializer(), tf.local_variables_initializer())
    sess.run(init_op)

    # 启动多线程处理输入数据
    coord = tf.train.Coordinator()
    threads = tf.train.start_queue_runners(coord=coord)

    ....


    #关闭线程
    coord.request_stop()
    coord.join(threads)
    sess.close()