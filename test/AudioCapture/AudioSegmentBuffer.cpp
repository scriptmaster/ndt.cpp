#include "AudioSegmentBuffer.h"
#include <algorithm>

AudioSegmentBuffer::AudioSegmentBuffer(int sampleRate,
                                     double prePaddingMs,
                                     double postPaddingMs)
    : sampleRate_(sampleRate),
      prePaddingMs_(prePaddingMs),
      postPaddingMs_(postPaddingMs),
      segmentReady_(false),
      wasSpeaking_(false),
      totalSamplesCollected_(0) {
}

size_t AudioSegmentBuffer::size() const {
    static int callCount = 0;
    callCount++;
    if (callCount % 100 == 0) {  // Debug every 100th call
        std::cout << "[DEBUG] AudioSegmentBuffer::size() called, buffer_.size() = " << buffer_.size() << std::endl;
    }

    size_t s = buffer_.size();
    // Sanity check - buffer should never be unreasonably large
    if (s > 1000000) {
        std::cout << "[WARNING] AudioSegmentBuffer size too large: " << s << std::endl;
        return 0; // Return 0 for corrupted size
    }
    return s;
}

void AudioSegmentBuffer::addSamples(const int16_t* samples, int numSamples, bool isSpeaking) {
    if (isSpeaking) {
        // Currently speaking - add samples to main buffer
        buffer_.insert(buffer_.end(), samples, samples + numSamples);
        totalSamplesCollected_ += numSamples;
        
        // If we just started speaking, try to add pre-padding
        if (!wasSpeaking_ && !prePaddingBuffer_.empty()) {
            addPrePadding();
        }
        
        wasSpeaking_ = true;
    } else {
        // Not speaking - maintain pre-padding buffer for next speech segment
        if (wasSpeaking_) {
            // Just transitioned from speaking to silence
            // Keep last samples in pre-padding buffer for next segment
            int paddingSize = samplesForDuration(prePaddingMs_);
            int keepSize = std::min(paddingSize, static_cast<int>(buffer_.size()));
            
            prePaddingBuffer_.clear();
            if (keepSize > 0) {
                auto startIt = buffer_.end() - keepSize;
                prePaddingBuffer_.insert(prePaddingBuffer_.end(), startIt, buffer_.end());
            }
            
            buffer_.clear();
        } else {
            // Still not speaking - maintain pre-padding buffer
            int paddingSize = samplesForDuration(prePaddingMs_);
            
            // Add new samples to pre-padding buffer
            prePaddingBuffer_.insert(prePaddingBuffer_.end(), samples, samples + numSamples);
            
            // Keep only last N samples (sliding window)
            if (prePaddingBuffer_.size() > static_cast<size_t>(paddingSize)) {
                prePaddingBuffer_.erase(prePaddingBuffer_.begin(),
                    prePaddingBuffer_.end() - paddingSize);
            }
        }
        
        wasSpeaking_ = false;
    }
}

void AudioSegmentBuffer::finalizeSegment() {
    if (buffer_.empty()) {
        segmentReady_ = false;
        return;
    }
    
    // Add post-padding if we have recent samples
    addPostPadding();
    
    segmentReady_ = true;
}

std::vector<int16_t> AudioSegmentBuffer::consumeSegment() {
    if (!segmentReady_ || buffer_.empty()) {
        return std::vector<int16_t>();
    }
    
    std::vector<int16_t> result = std::move(buffer_);
    buffer_.clear();
    prePaddingBuffer_.clear();
    segmentReady_ = false;
    wasSpeaking_ = false;
    
    return result;
}

void AudioSegmentBuffer::clear() {
    buffer_.clear();
    prePaddingBuffer_.clear();
    segmentReady_ = false;
    wasSpeaking_ = false;
}

void AudioSegmentBuffer::addPrePadding() {
    if (prePaddingBuffer_.empty()) {
        return;
    }
    
    // Insert pre-padding at the beginning of buffer
    buffer_.insert(buffer_.begin(), prePaddingBuffer_.begin(), prePaddingBuffer_.end());
    prePaddingBuffer_.clear();
}

void AudioSegmentBuffer::addPostPadding() {
    // Post-padding is handled by keeping samples after speech ends
    // This is done in addSamples() when transitioning from speaking to silence
    // For now, we could add silence samples, but it's better to use actual audio
    // if available, which is already handled by the pre-padding mechanism
}
