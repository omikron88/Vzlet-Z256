#include "vz256/wd2797.hpp"

#include <algorithm>

namespace vz256 {
namespace {

std::uint32_t byte_delay(const FloppyImage& drive) {
    return drive.geometry().encoding == FloppyEncoding::mfm ? 64U : 128U;
}

std::uint8_t sector_size_code(std::size_t size) {
    std::uint8_t code = 0;
    for (std::size_t value = 128; value < size; value *= 2) ++code;
    return code;
}

std::uint16_t crc16(std::span<const std::uint8_t> bytes) {
    std::uint16_t crc = 0xffff;
    for (const auto byte : bytes) {
        crc ^= static_cast<std::uint16_t>(byte) << 8U;
        for (unsigned bit = 0; bit < 8; ++bit) {
            crc = static_cast<std::uint16_t>((crc & 0x8000U) != 0
                ? (crc << 1U) ^ 0x1021U : crc << 1U);
        }
    }
    return crc;
}

void append_crc(std::vector<std::uint8_t>& output, std::size_t begin) {
    const auto crc = crc16(std::span<const std::uint8_t>{output}.subspan(begin));
    output.push_back(static_cast<std::uint8_t>(crc >> 8U));
    output.push_back(static_cast<std::uint8_t>(crc));
}

} // namespace

void Wd2797::reset() {
    status_ = track_zero;
    track_ = 0;
    sector_ = 1;
    data_ = command_ = 0;
    drq_ = intrq_ = index_ = multiple_ = false;
    side_ = 0;
    type_one_status_ = true;
    transfer_ = Transfer::none;
    buffer_.clear();
    position_ = 0;
    drq_delay_ = 0;
}

void Wd2797::tick(std::uint32_t cycles) {
    if (drq_delay_ == 0) return;
    if (cycles >= drq_delay_) {
        drq_delay_ = 0;
        drq_ = true;
    } else {
        drq_delay_ -= cycles;
    }
}

std::uint8_t Wd2797::status(std::array<FloppyImage, 4>& drives, std::uint8_t drive) {
    auto result = status_;
    if (!drives[drive].mounted()) result |= not_ready;
    if (drq_) result |= data_request;
    if (type_one_status_ && track_ == 0) result |= track_zero;
    // The boot monitor waits for an index edge before issuing RESTORE.
    if (type_one_status_) {
        index_ = !index_;
        if (transfer_ == Transfer::none && index_) result |= data_request;
    }
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
        else drq_delay_ = byte_delay(drives[drive]);
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
        else drq_delay_ = byte_delay(drives[drive]);
        break;
    }
}

bool Wd2797::begin_sector(std::array<FloppyImage, 4>& drives, std::uint8_t drive) {
    if (!drives[drive].mounted()) {
        status_ = not_ready;
        transfer_ = Transfer::none;
        drq_ = false;
        intrq_ = true;
        return false;
    }
    const auto bytes = drives[drive].sector(track_, side_, sector_);
    if (bytes.empty()) {
        status_ = record_not_found;
        transfer_ = Transfer::none;
        drq_ = false;
        intrq_ = true;
        return false;
    }
    buffer_.assign(drives[drive].geometry().sector_size, 0);
    if (transfer_ == Transfer::read) std::copy(bytes.begin(), bytes.end(), buffer_.begin());
    position_ = 0;
    status_ = busy;
    drq_ = true;
    drq_delay_ = 0;
    intrq_ = false;
    return true;
}

bool Wd2797::begin_read_track(std::array<FloppyImage, 4>& drives, std::uint8_t drive) {
    const auto& image = drives[drive];
    const auto& geometry = image.geometry();
    if (track_ >= geometry.cylinders || side_ >= geometry.sides) {
        status_ = record_not_found;
        intrq_ = true;
        return false;
    }

    buffer_.clear();
    const bool mfm = geometry.encoding == FloppyEncoding::mfm;
    for (std::size_t number = 1; number <= geometry.sectors_per_track; ++number) {
        const auto bytes = image.sector(track_, side_, number);
        buffer_.insert(buffer_.end(), mfm ? 40U : 20U, mfm ? 0x4eU : 0xffU);
        buffer_.insert(buffer_.end(), mfm ? 12U : 6U, 0x00U);

        auto mark = buffer_.size();
        if (mfm) buffer_.insert(buffer_.end(), 3, 0xa1U);
        buffer_.push_back(0xfeU);
        buffer_.push_back(track_);
        buffer_.push_back(side_);
        buffer_.push_back(static_cast<std::uint8_t>(number));
        buffer_.push_back(sector_size_code(geometry.sector_size));
        append_crc(buffer_, mark);

        buffer_.insert(buffer_.end(), mfm ? 22U : 11U, mfm ? 0x4eU : 0xffU);
        buffer_.insert(buffer_.end(), mfm ? 12U : 6U, 0x00U);
        mark = buffer_.size();
        if (mfm) buffer_.insert(buffer_.end(), 3, 0xa1U);
        buffer_.push_back(0xfbU);
        buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
        append_crc(buffer_, mark);
    }
    buffer_.insert(buffer_.end(), mfm ? 80U : 40U, mfm ? 0x4eU : 0xffU);
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
        // A real WD2797 keeps searching for the next ID field after the last
        // sector. It does not report RNF at the instant the final data byte is
        // consumed; the BIOS uses that interval to issue Force Interrupt.
        if (sector_ >= drives[drive].geometry().sectors_per_track) {
            status_ = busy;
            drq_ = false;
            drq_delay_ = 0;
            return;
        }
        ++sector_;
        if (begin_sector(drives, drive)) {
            drq_ = false;
            // Leave a rotational gap long enough for the BIOS transfer loop
            // to distinguish the end of one physical sector from the next.
            drq_delay_ = 1'024;
            return;
        }
    }
    status_ = 0;
    transfer_ = Transfer::none;
    drq_ = false;
    drq_delay_ = 0;
    intrq_ = true;
}

void Wd2797::command(std::uint8_t value, std::array<FloppyImage, 4>& drives,
                     std::uint8_t drive) {
    command_ = value;
    drq_ = intrq_ = false;
    drq_delay_ = 0;
    transfer_ = Transfer::none;
    status_ = 0;

    const auto type = static_cast<std::uint8_t>(value & 0xF0U);
    if (type == 0xD0U) { // Force Interrupt
        intrq_ = (value & 0x0FU) != 0;
        return;
    }
    if (type < 0x80U) { // Type I seek/step commands complete immediately.
        type_one_status_ = true;
        if (type == 0x00U) track_ = 0;
        else if (type == 0x10U) track_ = data_;
        else if (type == 0x40U || type == 0x50U) ++track_;
        else if ((type == 0x60U || type == 0x70U) && track_ != 0) --track_;
        intrq_ = true;
        return;
    }

    side_ = static_cast<std::uint8_t>((value >> 1U) & 1U);
    type_one_status_ = false;
    multiple_ = (value & 0x10U) != 0;
    if (!drives[drive].mounted()) {
        status_ = not_ready;
        intrq_ = true;
        return;
    }
    if ((type & 0xC0U) == 0x80U) {
        transfer_ = (value & 0x20U) != 0 ? Transfer::write : Transfer::read;
        begin_sector(drives, drive);
    } else if (type == 0xC0U) {
        transfer_ = Transfer::read_address;
        buffer_ = {track_, side_, sector_,
                   sector_size_code(drives[drive].geometry().sector_size)};
        std::vector<std::uint8_t> id_field;
        if (drives[drive].geometry().encoding == FloppyEncoding::mfm) {
            id_field.insert(id_field.end(), 3, 0xa1U);
        }
        id_field.push_back(0xfeU);
        id_field.insert(id_field.end(), buffer_.begin(), buffer_.end());
        const auto crc = crc16(id_field);
        buffer_.push_back(static_cast<std::uint8_t>(crc >> 8U));
        buffer_.push_back(static_cast<std::uint8_t>(crc));
        position_ = 0;
        status_ = busy;
        drq_ = true;
    } else if (type == 0xE0U) {
        transfer_ = Transfer::read_track;
        begin_read_track(drives, drive);
    } else {
        status_ = record_not_found;
        intrq_ = true;
    }
}

} // namespace vz256
