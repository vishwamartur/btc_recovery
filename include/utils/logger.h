#pragma once

#include <string>
#include <fstream>
#include <mutex>
#include <memory>

/**
 * Log levels
 */
enum class LogLevel {
    DEBUG = 0,
    INFO = 1,
    WARN = 2,
    ERROR = 3
};

/**
 * Thread-safe logger class
 */
class Logger {
public:
    /**
     * Initialize the logger
     * @param level Log level string ("debug", "info", "warn", "error")
     * @param console_output Enable console output
     * @param log_file Optional log file path
     */
    static void initialize(const std::string& level, bool console_output = true, 
                          const std::string& log_file = "");

    /**
     * Log messages at different levels
     */
    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warn(const std::string& message);
    static void error(const std::string& message);

    /**
     * Log with custom level
     */
    static void log(LogLevel level, const std::string& message);

    /**
     * Set log level
     */
    static void set_level(LogLevel level);
    static void set_level(const std::string& level);

    /**
     * Enable/disable console output
     */
    static void set_console_output(bool enabled);

    /**
     * Set log file
     */
    static void set_log_file(const std::string& file_path);

    /**
     * Flush all pending log messages
     */
    static void flush();

    /**
     * Shutdown logger
     */
    static void shutdown();

private:
    static std::unique_ptr<Logger> instance_;
    static std::mutex mutex_;

    LogLevel current_level_;
    bool console_output_;
    std::string log_file_path_;
    std::ofstream log_file_;
    std::mutex log_mutex_;

    Logger();
    ~Logger();

    void write_log(LogLevel level, const std::string& message);
    std::string get_timestamp() const;
    std::string level_to_string(LogLevel level) const;
    LogLevel string_to_level(const std::string& level) const;

    static Logger& get_instance();
};
