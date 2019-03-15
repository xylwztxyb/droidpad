//
// Created by xue on 19-1-6.
//

#include "caffe_classifier.h"
#include <android/log.h>

#define LOG_E(...) __android_log_print(ANDROID_LOG_ERROR, "caffe_classifier", __VA_ARGS__)

static bool PairCompare(const std::pair<float, int> &lhs,
                        const std::pair<float, int> &rhs) {
    return lhs.first > rhs.first;
}

/* Return the indices of the top N values of vector v. */
static std::vector<int> Argmax(const std::vector<float> &v, int N) {
    std::vector<std::pair<float, int> > pairs;
    for (size_t i = 0; i < v.size(); ++i)
        pairs.push_back(std::make_pair(v[i], i));
    std::partial_sort(pairs.begin(), pairs.begin() + N, pairs.end(), PairCompare);

    std::vector<int> result;
    for (int i = 0; i < N; ++i)
        result.push_back(pairs[i].second);
    return result;
}

////////////////////////////////////////////////////////

bool Classifier::init(const string &model_file, const string &trained_file, const string &mean_file,
                      const string &label_file) {
    Caffe::set_mode(Caffe::CPU);

    /* Load the network. */
    net_.reset(new Net<float>(model_file, TEST));
    net_->CopyTrainedLayersFrom(trained_file);

    if (net_->num_inputs() != 1 || net_->num_outputs() != 1) {
        LOG_E("Network should have exactly one input and one output");
        return false;
    }

    Blob<float> *input_layer = net_->input_blobs()[0];
    num_channels_ = input_layer->channels();
    if (num_channels_ != 3 && num_channels_ != 1) {
        LOG_E("Input layer should have 1 or 3 channels. (%d)", num_channels_);
        return false;
    }
    input_geometry_ = cv::Size(input_layer->width(), input_layer->height());

    input_layer->Reshape(1, num_channels_,
                         input_geometry_.height, input_geometry_.width);
    /* Forward dimension change to all layers. */
    net_->Reshape();

    float *input_data = input_layer->mutable_cpu_data();
    input_channel_.push_back(
            cv::Mat(input_geometry_.height, input_geometry_.width, CV_32FC1, input_data));

    /* Load the binaryproto mean file. */
    if (!SetMean(mean_file))
        return false;

    /* Load labels. */
    std::ifstream labels(label_file.c_str());
    if (!labels) {
        LOG_E("Unable to open labels file.(%s)", label_file.c_str());
        return false;
    }
    string line;
    while (std::getline(labels, line))
        labels_.push_back(string(line));

    Blob<float> *output_layer = net_->output_blobs()[0];
    if (labels_.size() != output_layer->channels()) {
        LOG_E("Number of labels is different from the output layer dimension.(%d vs %d)",
              labels_.size(), output_layer->channels());
        return false;
    }

    return true;
}

/* Return the top N predictions. */
std::vector<Prediction> Classifier::Classify(const cv::Mat &img, int N) {
    std::vector<float> output = Predict(img);

    N = std::min<int>(labels_.size(), N);
    std::vector<int> maxN = Argmax(output, N);
    std::vector<Prediction> predictions;
    for (int i = 0; i < N; ++i) {
        int idx = maxN[i];
        predictions.push_back(std::make_pair(labels_[idx], output[idx]));
    }

    return predictions;
}

/* Load the mean file in binaryproto format. */
bool Classifier::SetMean(const string &mean_file) {
    BlobProto blob_proto;
    ReadProtoFromBinaryFileOrDie(mean_file.c_str(), &blob_proto);

    /* Convert from BlobProto to Blob<float> */
    Blob<float> mean_blob;
    mean_blob.FromProto(blob_proto);
    if (mean_blob.channels() != num_channels_) {
        LOG_E("Number of channels of mean file doesn't match input layer.");
        return false;
    }

    /* The format of the mean file is planar 32-bit float BGR or grayscale. */
    std::vector<cv::Mat> channels;
    float *data = mean_blob.mutable_cpu_data();
    for (int i = 0; i < num_channels_; ++i) {
        /* Extract an individual channel. */
        cv::Mat channel(mean_blob.height(), mean_blob.width(), CV_32FC1, data);
        channels.push_back(channel);
        data += mean_blob.height() * mean_blob.width();
    }

    /* Merge the separate channels into a single image. */
    cv::Mat mean;
    cv::merge(channels, mean);

    /* Compute the global mean pixel value and create a mean image
     * filled with this value. */
    cv::Scalar channel_mean = cv::mean(mean);
    mean_ = cv::Mat(input_geometry_, mean.type(), channel_mean);

    return true;
}

std::vector<float> Classifier::Predict(const cv::Mat &img) {
    Preprocess(img);
    net_->Forward();

    /* Copy the output layer to a std::vector */
    Blob<float> *output_layer = net_->output_blobs()[0];
    const float *begin = output_layer->cpu_data();
    const float *end = begin + output_layer->channels();
    return std::vector<float>(begin, end);
}

void Classifier::Preprocess(const cv::Mat &img) {
    /* Convert the input image to the input image format of the network. */
    //img requires to be gray, 227*227, CV_32FC1
    cv::Mat sample_normalized;
    cv::subtract(img, mean_, sample_normalized);

    /* This operation will write the separate BGR planes directly to the
     * input layer of the network because it is wrapped by the cv::Mat
     * objects in input_channels. */
    cv::split(sample_normalized, input_channel_);
}
