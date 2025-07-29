#include "utils/logger.h"
#include <iostream>
#include <chrono>
#include <iomanip>
#include <sstream>

std::unique_ptr<Logger> Logger::instance_ = nullptr;
std::mutex Logger::mutex_;

Logger::Logger() : current_level_(LogLevel::INFO), console_output_(true) {}

Logger::~Logger() {
    if (log_file_.is_open()) {
        log_file_.close();
    }
}

Logger& Logger::get_instance() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (!instance_) {
        instance_ = std::unique_ptr<Logger>(new Logger());
    }
    return *instance_;
}

void Logger::initialize(const std::string& level, bool console_output, const std::string& log_file) {
    auto& logger = get_instance();
    std::lock_guard<std::mutex> lock(logger.log_mutex_);
    
    logger.set_level(level);
    logger.console_output_ = console_output;
    
    if (!log_file.empty()) {
        logger.log_file_path_ = log_file;
        logger.log_file_.open(log_file, std::ios::app);
        if (!logger.log_file_.is_open()) {
            std::cerr << "Warning: Could not open log file: " << log_file << std::endl;
        }
    }
}

void Logger::debug(const std::string& message) {
    log(LogLevel::DEBUG, message);
}

void Logger::info(const std::string& message) {
    log(LogLevel::INFO, message);
}

void Logger::warn(const std::string& message) {
    log(LogLevel::WARN, message);
}

void Logger::error(const std::string& message) {
    log(LogLevel::ERROR, message);
}

void Logger::log(LogLevel level, const std::string& message) {
    auto& logger = get_instance();
    logger.write_log(level, message);
}

void Logger::set_level(LogLevel level) {
    auto& logger = get_instance();
    std::lock_guard<std::mutex> lock(logger.log_mutex_);
    logger.current_level_ = level;
}

void Logger::set_level(const std::string& level) {
    auto& logger = get_instance();
    std::lock_guard<std::mutex> lock(logger.log_mutex_);
    logger.current_level_ = logger.string_to_level(level);
}

void Logger::set_console_output(bool enabled) {
    auto& logger = get_instance();
    std::lock_guard<std::mutex> lock(logger.log_mutex_);
    logger.console_output_ = enabled;
}

void Logger::set_log_file(const std::string& file_path) {
    auto& logger = get_instance();
    std::lock_guard<std::mutex> lock(logger.log_mutex_);
    
    if (logger.log_file_.is_open()) {
        logger.log_file_.close();
    }
    
    logger.log_file_path_ = file_path;
    if (!file_path.empty()) {
        logger.log_file_.open(file_path, std::ios::app);
        if (!logger.log_file_.is_open()) {
            std::cerr << "Warning: Could not open log file: " << file_path << std::endl;
        }
    }
}

void Logger::flush() {
    auto& logger = get_instance();
    std::lock_guard<std::mutex> lock(logger.log_mutex_);
    
    if (logger.console_output_) {
        std::cout.flush();
        std::cerr.flush();
    }
    
    if (logger.log_file_.is_open()) {
        logger.log_file_.flush();
    }
}

void Logger::shutdown() {
    std::lock_guard<std::mutex> lock(mutex_);
    if (instance_) {
        std::lock_guard<std::mutex> log_lock(instance_->log_mutex_);
        if (instance_->log_file_.is_open()) {
            instance_->log_file_.close();
        }
        instance_.reset();
    }
}

void Logger::write_log(LogLevel level, const std::string& message) {
    std::lock_guard<std::mutex> lock(log_mutex_);
    
    if (level < current_level_) {
        return;
    }
    
    std::string timestamp = get_timestamp();
    std::string level_str = level_to_string(level);
    std::string formatted_message = "[" + timestamp + "] [" + level_str + "] " + message;
    
    if (console_output_) {
        if (level >= LogLevel::ERROR) {
            std::cerr << formatted_message << std::endl;
        } else {
            std::cout << formatted_message << std::endl;
        }
    }
    
    if (log_file_.is_open()) {
        log_file_ << formatted_message << std::endl;
    }
}

std::string Logger::get_timestamp() const {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()) % 1000;
    
    std::stringstream ss;
    ss << std::put_time(std::localtime(&time_t), "%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    
    return ss.str();
}

std::string Logger::level_to_string(LogLevel level) const {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO:  return "INFO ";
        case LogLevel::WARN:  return "WARN ";
        case LogLevel::ERROR: return "ERROR";
        default: return "UNKNOWN";
    }
}

LogLevel Logger::string_to_level(const std::string& level) const {
    std::string lower_level = level;
    std::transform(lower_level.begin(), lower_level.end(), lower_level.begin(), ::tolower);
    
    if (lower_level == "debug") return LogLevel::DEBUG;
    if (lower_level == "info") return LogLevel::INFO;
    if (lower_level == "warn" || lower_level == "warning") return LogLevel::WARN;
    if (lower_level == "error") return LogLevel::ERROR;
    
    return LogLevel::INFO; // default
}
