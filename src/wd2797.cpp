#include "vz256/wd2797.hpp"

#include <algorithm>

namespace vz256 {

Wd2797::Activity Wd2797::activity() const {
    switch (transfer_) {
    case Transfer::read: return Activity::read;
    case Transfer::write: return Activity::write;
    case Transfer::read_address: return Activity::read_address;
    case Transfer::read_track: return Activity::read_track;
    case Transfer::write_track: return Activity::write_track;
    case Transfer::none: return Activity::idle;
    }
    return Activity::idle;
}
namespace {

constexpr std::uint32_t cpu_hz = 4'000'000;
constexpr std::uint32_t index_pulse_cycles = cpu_hz / 250; // approximately 4 ms

std::uint32_t byte_delay(const FloppyImage& drive) {
    // At a 4 MHz CPU clock the WD2797 service windows are 64 cycles for
    // 8-inch MFM, 128 for 8-inch FM or 5.25-inch MFM, and 256 for 5.25-inch
    // FM. The controller clock halves on a 5.25-inch drive and FM halves the
    // data rate once more.
    const auto& geometry = drive.geometry();
    const auto drive_divisor = geometry.rpm == 360 ? 1U : 2U;
    const auto density_divisor = geometry.encoding == FloppyEncoding::mfm ? 1U : 2U;
    return 64U * drive_divisor * density_divisor;
}

std::uint32_t head_load_delay(const FloppyImage& drive, std::uint8_t command) {
    if ((command & 0x04U) == 0) return 0;
    // WD2797 E flag: 15 ms for 8-inch drives, doubled by the 5.25-inch clock.
    return drive.geometry().rpm == 360 ? cpu_hz * 15U / 1'000U
                                       : cpu_hz * 30U / 1'000U;
}

std::uint32_t step_delay(const FloppyImage& drive, std::uint8_t command) {
    constexpr std::array<std::uint32_t, 4> eight_inch_ms{3, 6, 10, 15};
    constexpr std::array<std::uint32_t, 4> five_inch_ms{6, 12, 20, 30};
    const auto& rates = drive.mounted() && drive.geometry().rpm == 360
        ? eight_inch_ms : five_inch_ms;
    return rates[command & 3U] * cpu_hz / 1'000U;
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
    head_tracks_.fill(0);
    sector_ = 1;
    data_ = command_ = 0;
    drq_ = intrq_ = index_ = interrupt_on_index_ = false;
    interrupt_on_ready_ = interrupt_on_not_ready_ = ready_ = multiple_ = false;
    head_loaded_ = false;
    step_in_ = true;
    side_ = 0;
    type_one_status_ = true;
    transfer_ = Transfer::none;
    buffer_.clear();
    position_ = 0;
    drq_delay_ = 0;
    drq_timeout_ = 0;
    type_one_delay_ = 0;
    type_one_origin_track_ = 0;
    type_one_origin_head_track_ = 0;
    rotation_period_cycles_ = cpu_hz / 5; // 300 RPM
    rotation_phase_cycles_ = 0;
    format_state_ = FormatState::search_id;
    format_id_position_ = 0;
    format_data_.clear();
    formatted_sector_seen_.clear();
    formatted_sectors_ = 0;
}

void Wd2797::tick(std::uint32_t cycles, const std::array<FloppyImage, 4>& drives,
                  std::uint8_t drive) {
    const bool ready = drives[drive].mounted();
    if (ready != ready_) {
        if ((ready && interrupt_on_ready_) || (!ready && interrupt_on_not_ready_)) {
            intrq_ = true;
        }
        ready_ = ready;
    }
    const auto rotation_total = static_cast<std::uint64_t>(rotation_phase_cycles_) + cycles;
    const bool index_edge = rotation_total >= rotation_period_cycles_;
    rotation_phase_cycles_ = static_cast<std::uint32_t>(
        rotation_total % rotation_period_cycles_);
    if (index_edge && interrupt_on_index_) intrq_ = true;
    if (type_one_delay_ != 0) {
        if (cycles < type_one_delay_) {
            type_one_delay_ -= cycles;
        } else {
            type_one_delay_ = 0;
            status_ &= static_cast<std::uint8_t>(~busy);
            intrq_ = true;
        }
    }
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

void Wd2797::set_rotation_speed(const FloppyImage& drive) {
    if (!drive.mounted()) return;
    const auto period = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(cpu_hz) * 60U) / drive.geometry().rpm);
    if (period != rotation_period_cycles_) {
        rotation_period_cycles_ = period;
        rotation_phase_cycles_ %= rotation_period_cycles_;
    }
}

std::uint8_t Wd2797::status(std::array<FloppyImage, 4>& drives, std::uint8_t drive) {
    set_rotation_speed(drives[drive]);
    auto result = status_;
    if (!drives[drive].mounted()) result |= not_ready;
    if (drq_) result |= data_request;
    if (type_one_status_ && head_tracks_[drive] == 0) result |= track_zero;
    if (type_one_status_ && head_loaded_) result |= head_loaded;
    if (type_one_status_ && drives[drive].write_protected()) result |= write_protect;
    // Bit 1 is INDEX for Type I commands and DRQ for Type II/III commands.
    // The pulse is derived from the selected medium's rotational speed rather
    // than from how often software happens to read the status register.
    index_ = drives[drive].mounted() && rotation_phase_cycles_ < index_pulse_cycles;
    if (type_one_status_ && index_) result |= data_request;
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
    const auto head_track = head_tracks_[drive];
    if (track_ != head_track) {
        status_ = record_not_found;
        transfer_ = Transfer::none;
        drq_ = false;
        intrq_ = true;
        return false;
    }
    const auto slot = drives[drive].sector_slot_by_id(head_track, side_, sector_);
    const auto bytes = drives[drive].sector_by_id(head_track, side_, sector_);
    if (bytes.empty()) {
        status_ = record_not_found;
        transfer_ = Transfer::none;
        drq_ = false;
        intrq_ = true;
        return false;
    }
    buffer_.assign(bytes.size(), 0);
    current_sector_metadata_ = drives[drive].sector_metadata(head_track, side_, slot + 1);
    if (transfer_ == Transfer::read) std::copy(bytes.begin(), bytes.end(), buffer_.begin());
    position_ = 0;
    status_ = busy;
    const auto sector_period = rotation_period_cycles_ /
                               drives[drive].geometry().sectors_per_track;
    const auto target_phase = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(index_pulse_cycles) +
         static_cast<std::uint64_t>(slot) * sector_period) %
        rotation_period_cycles_);
    const auto settle = head_load_delay(drives[drive], command_);
    const auto settled_phase = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(rotation_phase_cycles_) + settle) %
        rotation_period_cycles_);
    const auto rotational_wait = target_phase >= settled_phase
        ? target_phase - settled_phase
        : rotation_period_cycles_ - settled_phase + target_phase;
    drq_delay_ = settle + rotational_wait;
    drq_ = drq_delay_ == 0;
    drq_timeout_ = byte_delay(drives[drive]);
    intrq_ = false;
    return true;
}

bool Wd2797::begin_read_address(std::array<FloppyImage, 4>& drives,
                                std::uint8_t drive) {
    const auto& image = drives[drive];
    const auto& geometry = image.geometry();
    const auto head_track = head_tracks_[drive];
    if (head_track >= geometry.cylinders || side_ >= geometry.sides ||
        geometry.sectors_per_track == 0) {
        status_ = record_not_found;
        transfer_ = Transfer::none;
        intrq_ = true;
        return false;
    }

    const auto settle = head_load_delay(image, command_);
    const auto settled_phase = static_cast<std::uint32_t>(
        (static_cast<std::uint64_t>(rotation_phase_cycles_) + settle) %
        rotation_period_cycles_);
    const auto sector_period = rotation_period_cycles_ / geometry.sectors_per_track;
    std::uint32_t shortest_wait = rotation_period_cycles_;
    std::size_t next_slot = 0;
    for (std::uint32_t number = 1; number <= geometry.sectors_per_track; ++number) {
        const auto target_phase = static_cast<std::uint32_t>(
            (static_cast<std::uint64_t>(index_pulse_cycles) +
             static_cast<std::uint64_t>(number - 1U) * sector_period) %
            rotation_period_cycles_);
        const auto wait = target_phase >= settled_phase
            ? target_phase - settled_phase
            : rotation_period_cycles_ - settled_phase + target_phase;
        if (wait < shortest_wait) {
            shortest_wait = wait;
            next_slot = number - 1;
        }
    }

    sector_ = image.sector_metadata(head_track, side_, next_slot + 1).id;
    const auto address_metadata = image.sector_metadata(head_track, side_, next_slot + 1);
    buffer_ = {head_track, side_, sector_, sector_size_code(address_metadata.size)};
    std::vector<std::uint8_t> id_field;
    if (geometry.encoding == FloppyEncoding::mfm) id_field.insert(id_field.end(), 3, 0xa1U);
    id_field.push_back(0xfeU);
    id_field.insert(id_field.end(), buffer_.begin(), buffer_.end());
    const auto crc = crc16(id_field);
    buffer_.push_back(static_cast<std::uint8_t>(crc >> 8U));
    buffer_.push_back(static_cast<std::uint8_t>(crc));
    position_ = 0;
    status_ = busy;
    drq_delay_ = settle + shortest_wait;
    drq_ = drq_delay_ == 0;
    drq_timeout_ = byte_delay(image);
    intrq_ = false;
    return true;
}

bool Wd2797::begin_read_track(std::array<FloppyImage, 4>& drives, std::uint8_t drive) {
    const auto& image = drives[drive];
    const auto& geometry = image.geometry();
    const auto head_track = head_tracks_[drive];
    if (head_track >= geometry.cylinders || side_ >= geometry.sides) {
        status_ = record_not_found;
        transfer_ = Transfer::none;
        intrq_ = true;
        return false;
    }

    buffer_.clear();
    const bool mfm = geometry.encoding == FloppyEncoding::mfm;
    for (std::size_t number = 1; number <= geometry.sectors_per_track; ++number) {
        const auto bytes = image.sector(head_track, side_, number);
        buffer_.insert(buffer_.end(), mfm ? 40U : 20U, mfm ? 0x4eU : 0xffU);
        buffer_.insert(buffer_.end(), mfm ? 12U : 6U, 0x00U);

        auto mark = buffer_.size();
        if (mfm) buffer_.insert(buffer_.end(), 3, 0xa1U);
        buffer_.push_back(0xfeU);
        buffer_.push_back(head_track);
        buffer_.push_back(side_);
        const auto metadata = image.sector_metadata(head_track, side_, number);
        buffer_.push_back(metadata.id);
        buffer_.push_back(sector_size_code(metadata.size));
        append_crc(buffer_, mark);

        buffer_.insert(buffer_.end(), mfm ? 22U : 11U, mfm ? 0x4eU : 0xffU);
        buffer_.insert(buffer_.end(), mfm ? 12U : 6U, 0x00U);
        mark = buffer_.size();
        if (mfm) buffer_.insert(buffer_.end(), 3, 0xa1U);
        buffer_.push_back(metadata.deleted_data ? 0xf8U : 0xfbU);
        buffer_.insert(buffer_.end(), bytes.begin(), bytes.end());
        append_crc(buffer_, mark);
    }
    buffer_.insert(buffer_.end(), mfm ? 80U : 40U, mfm ? 0x4eU : 0xffU);
    position_ = 0;
    status_ = busy;
    drq_delay_ = head_load_delay(image, command_);
    drq_ = drq_delay_ == 0;
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
    if (head_tracks_[drive] >= geometry.cylinders || side_ >= geometry.sides) {
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
    drq_delay_ = head_load_delay(image, command_);
    drq_ = drq_delay_ == 0;
    drq_timeout_ = byte_delay(image);
    intrq_ = false;
    return true;
}

void Wd2797::write_track_byte(std::uint8_t value,
                              std::array<FloppyImage, 4>& drives,
                              std::uint8_t drive) {
    auto& image = drives[drive];
    const auto& geometry = image.geometry();
    const auto head_track = head_tracks_[drive];
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
            if (format_id_[0] != head_track || format_id_[1] != side_ ||
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
            !image.write_sector(head_track, side_, format_id_[2], format_data_)) {
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
        !drives[drive].write_sector(head_tracks_[drive], side_, sector_, buffer_)) {
        status_ = drives[drive].write_protected() ? write_protect : record_not_found;
        transfer_ = Transfer::none;
        intrq_ = true;
        return;
    }
    if (multiple_) {
        // A real WD2797 keeps searching for the next ID field after the last
        // sector. It does not report RNF at the instant the final data byte is
        // consumed; the BIOS uses that interval to issue Force Interrupt.
        const auto slot = drives[drive].sector_slot_by_id(head_tracks_[drive], side_, sector_);
        if (slot + 1 >= drives[drive].geometry().sectors_per_track) {
            status_ = busy;
            drq_ = false;
            drq_delay_ = 0;
            drq_timeout_ = 0;
            return;
        }
        sector_ = drives[drive].sector_metadata(head_tracks_[drive], side_, slot + 2).id;
        if (begin_sector(drives, drive)) {
            // begin_sector waits for the angular position of the following
            // sector, naturally exposing the inter-sector gap to the BIOS.
            return;
        }
    }
    status_ = transfer_ == Transfer::read
        ? static_cast<std::uint8_t>((current_sector_metadata_.crc_error ? crc_error : 0) |
                                    (current_sector_metadata_.deleted_data ? record_type : 0))
        : 0;
    transfer_ = Transfer::none;
    drq_ = false;
    drq_delay_ = 0;
    drq_timeout_ = 0;
    type_one_delay_ = 0;
    intrq_ = true;
}

void Wd2797::command(std::uint8_t value, std::array<FloppyImage, 4>& drives,
                     std::uint8_t drive) {
    auto& head_track = head_tracks_[drive];
    set_rotation_speed(drives[drive]);
    ready_ = drives[drive].mounted();
    const auto type = static_cast<std::uint8_t>(value & 0xF0U);
    // The command register is not receptive to another mechanical Type I
    // command while a seek/step is in progress. Force Interrupt remains the
    // asynchronous escape path; data commands retain the existing pragmatic
    // behavior used by the BIOS and commit the visible target position.
    if (type_one_delay_ != 0 && type < 0x80U) return;
    command_ = value;
    drq_ = intrq_ = false;
    interrupt_on_index_ = false;
    interrupt_on_ready_ = interrupt_on_not_ready_ = false;
    drq_delay_ = 0;
    drq_timeout_ = 0;
    // A Type I movement remains provisional until its timer expires. Force
    // Interrupt restores the pre-command physical state; another operational
    // command commits the currently visible target but still cancels the old
    // timer so it cannot complete during a later data transfer.
    if (type_one_delay_ != 0 && type == 0xD0U) {
        track_ = type_one_origin_track_;
        head_track = type_one_origin_head_track_;
    }
    type_one_delay_ = 0;
    transfer_ = Transfer::none;
    status_ = 0;

    if (type == 0xD0U) { // Force Interrupt
        interrupt_on_ready_ = (value & 0x01U) != 0;
        interrupt_on_not_ready_ = (value & 0x02U) != 0;
        interrupt_on_index_ = (value & 0x04U) != 0;
        intrq_ = (value & 0x08U) != 0;
        return;
    }
    if (type < 0x80U) { // Type I seek/step commands complete after step timing.
        type_one_status_ = true;
        head_loaded_ = (value & 0x08U) != 0;
        const auto previous_head_track = head_track;
        type_one_origin_track_ = track_;
        type_one_origin_head_track_ = head_track;
        if (type == 0x00U) {
            track_ = head_track = 0;
            step_in_ = false;
        } else if (type == 0x10U) {
            step_in_ = data_ >= head_track;
            head_track = data_;
            track_ = data_;
        } else {
            if (type == 0x40U || type == 0x50U) step_in_ = true;
            else if (type == 0x60U || type == 0x70U) step_in_ = false;
            if (step_in_) {
                if (head_track != 0xffU) ++head_track;
                if ((value & 0x10U) != 0 && track_ != 0xffU) ++track_;
            } else {
                if (head_track != 0) --head_track;
                if ((value & 0x10U) != 0 && track_ != 0) --track_;
            }
        }
        // With V set the real controller reads an ID field after moving the
        // head. Raw sector images have canonical IDs, so verification succeeds
        // exactly when the physical cylinder exists and its C field agrees
        // with the track register. A missing medium fails verification too;
        // NOT READY is added when software reads the status register.
        if ((value & 0x04U) != 0 &&
            (!drives[drive].mounted() ||
             head_track >= drives[drive].geometry().cylinders ||
             track_ != head_track)) {
            status_ |= seek_error;
        }
        auto steps = previous_head_track > head_track
            ? previous_head_track - head_track : head_track - previous_head_track;
        // A Step command still emits one pulse when the head is already at a
        // physical limit, even though the observable cylinder cannot change.
        if (type >= 0x20U) steps = 1;
        type_one_delay_ = static_cast<std::uint32_t>(steps) * step_delay(drives[drive], value);
        if (type_one_delay_ != 0) status_ |= busy;
        else intrq_ = true;
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
        begin_read_address(drives, drive);
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
