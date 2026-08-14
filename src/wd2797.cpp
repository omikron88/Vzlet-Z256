#include "vz256/wd2797.hpp"

#include <algorithm>

namespace vz256 {

void Wd2797::reset() {
    status_ = track_zero;
    track_ = 0;
    sector_ = 1;
    data_ = command_ = 0;
    drq_ = intrq_ = index_ = multiple_ = false;
    side_ = 0;
    transfer_ = Transfer::none;
    buffer_.clear();
    position_ = 0;
}

std::uint8_t Wd2797::status(std::array<FloppyImage, 4>& drives, std::uint8_t drive) {
    auto result = status_;
    if (!drives[drive].mounted()) result |= not_ready;
    if (drq_) result |= data_request;
    if (track_ == 0) result |= track_zero;
    // The boot monitor waits for an index edge before issuing RESTORE.
    index_ = !index_;
    if (transfer_ == Transfer::none && index_) result |= data_request;
    intrq_ = false; // reading status acknowledges INTRQ
    return result;
}

std::uint8_t Wd2797::read(std::uint8_t reg, std::array<FloppyImage, 4>& drives,
                          std::uint8_t drive) {
    switch (reg & 3U) {
    case 0: return status(drives, drive);
    case 1: return track_;
    case 2: return sector_;
    default:
        if (!drq_ || transfer_ == Transfer::write || position_ >= buffer_.size()) return data_;
        data_ = buffer_[position_++];
        drq_ = false;
        if (position_ == buffer_.size()) finish_sector(drives, drive);
        else drq_ = true;
        return data_;
    }
}

void Wd2797::write(std::uint8_t reg, std::uint8_t value,
                   std::array<FloppyImage, 4>& drives, std::uint8_t drive) {
    switch (reg & 3U) {
    case 0: command(value, drives, drive); break;
    case 1: track_ = value; break;
    case 2: sector_ = value; break;
    default:
        data_ = value;
        if (!drq_ || transfer_ != Transfer::write || position_ >= buffer_.size()) break;
        buffer_[position_++] = value;
        drq_ = false;
        if (position_ == buffer_.size()) finish_sector(drives, drive);
        else drq_ = true;
        break;
    }
}

bool Wd2797::begin_sector(std::array<FloppyImage, 4>& drives, std::uint8_t drive) {
    const auto bytes = drives[drive].sector(track_, side_, sector_);
    if (bytes.empty()) {
        status_ = record_not_found;
        transfer_ = Transfer::none;
        drq_ = false;
        intrq_ = true;
        return false;
    }
    buffer_.assign(FloppyImage::sector_size, 0);
    if (transfer_ == Transfer::read) std::copy(bytes.begin(), bytes.end(), buffer_.begin());
    position_ = 0;
    status_ = busy;
    drq_ = true;
    intrq_ = false;
    return true;
}

void Wd2797::finish_sector(std::array<FloppyImage, 4>& drives, std::uint8_t drive) {
    if (transfer_ == Transfer::write &&
        !drives[drive].write_sector(track_, side_, sector_, buffer_)) {
        status_ = drives[drive].write_protected() ? write_protect : record_not_found;
        transfer_ = Transfer::none;
        intrq_ = true;
        return;
    }
    if (multiple_) {
        ++sector_;
        if (begin_sector(drives, drive)) return;
    }
    status_ = 0;
    transfer_ = Transfer::none;
    drq_ = false;
    intrq_ = true;
}

void Wd2797::command(std::uint8_t value, std::array<FloppyImage, 4>& drives,
                     std::uint8_t drive) {
    command_ = value;
    drq_ = intrq_ = false;
    transfer_ = Transfer::none;
    status_ = 0;

    const auto type = static_cast<std::uint8_t>(value & 0xF0U);
    if (type == 0xD0U) { // Force Interrupt
        intrq_ = (value & 0x0FU) != 0;
        return;
    }
    if (type < 0x80U) { // Type I seek/step commands complete immediately.
        if (type == 0x00U) track_ = 0;
        else if (type == 0x10U) track_ = data_;
        else if (type == 0x40U || type == 0x50U) ++track_;
        else if ((type == 0x60U || type == 0x70U) && track_ != 0) --track_;
        intrq_ = true;
        return;
    }

    side_ = static_cast<std::uint8_t>((value >> 1U) & 1U);
    multiple_ = (value & 0x10U) != 0;
    if ((type & 0xC0U) == 0x80U) {
        transfer_ = (value & 0x20U) != 0 ? Transfer::write : Transfer::read;
        begin_sector(drives, drive);
    } else if (type == 0xC0U) {
        transfer_ = Transfer::read_address;
        buffer_ = {track_, side_, sector_, 2, 0, 0}; // N=2 means 512 bytes
        position_ = 0;
        status_ = busy;
        drq_ = true;
    } else {
        status_ = record_not_found;
        intrq_ = true;
    }
}

} // namespace vz256
