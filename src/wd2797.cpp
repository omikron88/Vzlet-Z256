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
    drq_timeout_ = 0;
    format_state_ = FormatState::search_id;
    format_id_position_ = 0;
    format_data_.clear();
    formatted_sector_seen_.clear();
    formatted_sectors_ = 0;
}

void Wd2797::tick(std::uint32_t cycles) {
    if (transfer_ == Transfer::none) return;
    if (drq_) {
        if (drq_timeout_ != 0 && cycles >= drq_timeout_) {
            status_ = static_cast<std::uint8_t>((status_ & ~busy) | lost_data);
            transfer_ = Transfer::none;
            drq_ = false;
            drq_delay_ = drq_timeout_ = 0;
            intrq_ = true;
        } else if (drq_timeout_ != 0) {
            drq_timeout_ -= cycles;
        }
        return;
    }
    if (drq_delay_ == 0) return;
    if (cycles < drq_delay_) {
        drq_delay_ -= cycles;
        return;
    }

    const auto remaining = cycles - drq_delay_;
    drq_delay_ = 0;
    drq_ = true;
    if (drq_timeout_ != 0 && remaining >= drq_timeout_) {
        status_ = static_cast<std::uint8_t>((status_ & ~busy) | lost_data);
        transfer_ = Transfer::none;
        drq_ = false;
        drq_timeout_ = 0;
        intrq_ = true;
    } else if (drq_timeout_ != 0) {
        drq_timeout_ -= remaining;
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
        if (!drq_ || transfer_ == Transfer::write || transfer_ == Transfer::write_track ||
            position_ >= buffer_.size()) return data_;
        data_ = buffer_[position_++];
        drq_ = false;
        drq_timeout_ = 0;
        if (position_ == buffer_.size()) finish_sector(drives, drive);
        else drq_delay_ = drq_timeout_ = byte_delay(drives[drive]);
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
        if (transfer_ == Transfer::write_track) {
            if (drq_) write_track_byte(value, drives, drive);
            break;
        }
        data_ = value;
        if (!drq_ || transfer_ != Transfer::write || position_ >= buffer_.size()) break;
        buffer_[position_++] = value;
        drq_ = false;
        drq_timeout_ = 0;
        if (position_ == buffer_.size()) finish_sector(drives, drive);
        else drq_delay_ = drq_timeout_ = byte_delay(drives[drive]);
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
    drq_timeout_ = byte_delay(drives[drive]);
    intrq_ = false;
    return true;
}

bool Wd2797::begin_read_track(std::array<FloppyImage, 4>& drives, std::uint8_t drive) {
    const auto& image = drives[drive];
    const auto& geometry = image.geometry();
    if (track_ >= geometry.cylinders || side_ >= geometry.sides) {
        status_ = record_not_found;
        transfer_ = Transfer::none;
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
    drq_timeout_ = byte_delay(image);
    intrq_ = false;
    return true;
}

bool Wd2797::begin_write_track(std::array<FloppyImage, 4>& drives, std::uint8_t drive) {
    auto& image = drives[drive];
    const auto& geometry = image.geometry();
    if (image.write_protected()) {
        status_ = write_protect;
        transfer_ = Transfer::none;
        intrq_ = true;
        return false;
    }
    if (track_ >= geometry.cylinders || side_ >= geometry.sides) {
        status_ = record_not_found;
        transfer_ = Transfer::none;
        intrq_ = true;
        return false;
    }
    format_state_ = FormatState::search_id;
    format_id_position_ = 0;
    format_data_.clear();
    formatted_sector_seen_.assign(geometry.sectors_per_track, false);
    formatted_sectors_ = 0;
    status_ = busy;
    drq_ = true;
    drq_timeout_ = byte_delay(image);
    intrq_ = false;
    return true;
}

void Wd2797::write_track_byte(std::uint8_t value,
                              std::array<FloppyImage, 4>& drives,
                              std::uint8_t drive) {
    auto& image = drives[drive];
    const auto& geometry = image.geometry();
    drq_ = false;
    drq_timeout_ = 0;

    switch (format_state_) {
    case FormatState::search_id:
        if (value == 0xfeU) {
            format_state_ = FormatState::id;
            format_id_position_ = 0;
        }
        break;
    case FormatState::id:
        format_id_[format_id_position_++] = value;
        if (format_id_position_ == format_id_.size()) format_state_ = FormatState::search_data;
        break;
    case FormatState::search_data:
        if (value == 0xfeU) {
            format_state_ = FormatState::id;
            format_id_position_ = 0;
        } else if (value == 0xfbU || value == 0xf8U) {
            const auto size_code = format_id_[3];
            const auto size = size_code <= 3U ? (128U << size_code) : 0U;
            if (format_id_[0] != track_ || format_id_[1] != side_ ||
                format_id_[2] == 0 || format_id_[2] > geometry.sectors_per_track ||
                size != geometry.sector_size) {
                status_ = record_not_found;
                transfer_ = Transfer::none;
                intrq_ = true;
                return;
            }
            format_data_.clear();
            format_data_.reserve(size);
            format_state_ = FormatState::data;
        }
        break;
    case FormatState::data:
        format_data_.push_back(value);
        if (format_data_.size() == geometry.sector_size) format_state_ = FormatState::data_crc;
        break;
    case FormatState::data_crc:
        // F7 tells the WD2797 to write the generated CRC. Accepting only this
        // token also prevents a truncated field from changing the image.
        const auto sector_index = static_cast<std::size_t>(format_id_[2] - 1U);
        if (value != 0xf7U || formatted_sector_seen_[sector_index] ||
            !image.write_sector(track_, side_, format_id_[2], format_data_)) {
            status_ = image.write_protected() ? write_protect : record_not_found;
            transfer_ = Transfer::none;
            intrq_ = true;
            return;
        }
        formatted_sector_seen_[sector_index] = true;
        ++formatted_sectors_;
        format_state_ = FormatState::search_id;
        if (formatted_sectors_ == geometry.sectors_per_track) {
            status_ = 0;
            transfer_ = Transfer::none;
            intrq_ = true;
            return;
        }
        break;
    }
    drq_delay_ = drq_timeout_ = byte_delay(image);
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
            drq_timeout_ = 0;
            return;
        }
        ++sector_;
        if (begin_sector(drives, drive)) {
            drq_ = false;
            // Leave a rotational gap long enough for the BIOS transfer loop
            // to distinguish the end of one physical sector from the next.
            drq_delay_ = 1'024;
            drq_timeout_ = byte_delay(drives[drive]);
            return;
        }
    }
    status_ = 0;
    transfer_ = Transfer::none;
    drq_ = false;
    drq_delay_ = 0;
    drq_timeout_ = 0;
    intrq_ = true;
}

void Wd2797::command(std::uint8_t value, std::array<FloppyImage, 4>& drives,
                     std::uint8_t drive) {
    command_ = value;
    drq_ = intrq_ = false;
    drq_delay_ = 0;
    drq_timeout_ = 0;
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
        drq_timeout_ = byte_delay(drives[drive]);
    } else if (type == 0xE0U) {
        transfer_ = Transfer::read_track;
        begin_read_track(drives, drive);
    } else if (type == 0xF0U) {
        transfer_ = Transfer::write_track;
        begin_write_track(drives, drive);
    } else {
        status_ = record_not_found;
        intrq_ = true;
    }
}

} // namespace vz256
