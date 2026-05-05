#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdint>
#include <filesystem>
#include <iomanip>
#include <algorithm>
#include <cctype>
#include <limits>
#include <sstream>

namespace fs = std::filesystem;

// --- CONFIGURATION ---
const std::vector<std::pair<std::string, std::vector<std::string>>> EXTENSION_CATEGORIES = {
    {"Music",       {".mp3", ".wav", ".m4a", ".flac"}},
    {"Videos",      {".mp4", ".mkv", ".avi", ".mov", ".wmv"}},
    {"Images",      {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".svg", ".webp"}},
    {"Documents",   {".pdf", ".docx", ".txt", ".xlsx", ".pptx", ".doc"}},
    {"Archives",    {".zip", ".rar", ".7z", ".tar", ".gz"}},
    {"Code",        {".py", ".html", ".css", ".js", ".cpp", ".java", ".c"}},
    {"Executables", {".exe", ".msi", ".bat", ".sh"}},
    {"Misc",        {".json", ".log", ".aep", ".jar"}}
};

// --- HELPER FUNCTIONS ---
std::string to_lower(std::string str) {
    std::transform(str.begin(), str.end(), str.begin(),
                   [](unsigned char c){ return std::tolower(c); });
    return str;
}

std::string format_size(uintmax_t bytes) {
    double size = static_cast<double>(bytes);
    const char* units[] = {"B", "KB", "MB", "GB", "TB"};
    int unit_idx = 0;
    while (size >= 1024.0 && unit_idx < 4) {
        size /= 1024.0;
        unit_idx++;
    }
    std::ostringstream oss;
    oss << std::fixed << std::setprecision(2) << size << " " << units[unit_idx];
    return oss.str();
}

std::string trim(const std::string& text) {
    const auto start = text.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) return {};
    const auto end = text.find_last_not_of(" \t\r\n");
    return text.substr(start, end - start + 1);
}

// --- ORGANIZE FILES ---
void sort_by_extension(const fs::path& target_dir) {
    int moved_count = 0;
    int skipped_count = 0;

    std::cout << "Organizing files in: " << target_dir.string() << "...\n";

    for (const auto& entry : fs::directory_iterator(target_dir)) {
        if (!entry.is_regular_file()) continue;

        fs::path filepath = entry.path();
        std::string ext = to_lower(filepath.extension().string());
        bool moved = false;

        for (const auto& [category, extensions] : EXTENSION_CATEGORIES) {
            if (std::find(extensions.begin(), extensions.end(), ext) != extensions.end()) {
                fs::path target_folder = target_dir / category;
                fs::create_directories(target_folder);

                try {
                    // Skip if destination already exists to prevent overwrites
                    fs::path dest = target_folder / filepath.filename();
                    if (!fs::exists(dest)) {
                        fs::rename(filepath, dest);
                        moved_count++;
                        moved = true;
                    }
                } catch (const std::exception& e) {
                    std::cerr << "  Warning: Could not move " << filepath.filename() 
                              << " (" << e.what() << ")\n";
                }
                break;
            }
        }

        if (!moved) skipped_count++;
    }

    std::cout << "\n✅ Organization Complete!\n";
    std::cout << "   Files Moved: " << moved_count << "\n";
    if (skipped_count > 0) {
        std::cout << "   Files Skipped (Unknown type): " << skipped_count << "\n";
    }
}

// --- STORAGE DASHBOARD ---
void show_dashboard(const fs::path& target_dir) {
    struct CategoryStats { uintmax_t size = 0; int count = 0; };
    std::map<std::string, CategoryStats> stats;

    // Initialize all categories
    for (const auto& [cat, _] : EXTENSION_CATEGORIES) stats[cat] = {};
    stats["Others"] = {};

    uintmax_t total_size = 0;
    int total_files = 0;

    std::cout << "\n📊 Calculating storage analytics for: " << target_dir.string() << "...\n";

    // Recursive directory walk (matches Python's os.walk behavior)
    for (const auto& entry : fs::recursive_directory_iterator(target_dir, 
                                                              fs::directory_options::skip_permission_denied)) {
        if (!entry.is_regular_file()) continue;

        try {
            uintmax_t fsize = entry.file_size();
            total_size += fsize;
            total_files++;

            // Categorize based on parent directory name (same logic as Python version)
            std::string parent_name = entry.path().parent_path().filename().string();
            bool found = false;
            for (const auto& [cat, _] : EXTENSION_CATEGORIES) {
                if (parent_name == cat) {
                    stats[cat].count++;
                    stats[cat].size += fsize;
                    found = true;
                    break;
                }
            }
            if (!found) {
                stats["Others"].count++;
                stats["Others"].size += fsize;
            }
        } catch (const std::exception&) {
            continue; // Ignore broken symlinks or inaccessible files
        }
    }

    // Print formatted table
    std::cout << "\n" << std::string(45, '=') << "\n";
    std::cout << std::left << std::setw(18) << "Category" 
              << std::right << std::setw(10) << "Count" 
              << std::setw(15) << "Total Size" << "\n";
    std::cout << std::string(45, '-') << "\n";

    for (const auto& [cat, data] : stats) {
        if (data.count > 0) {
            std::cout << std::left << std::setw(18) << cat 
                      << std::right << std::setw(10) << data.count 
                      << std::setw(15) << format_size(data.size) << "\n";
        }
    }

    std::cout << std::string(45, '-') << "\n";
    std::cout << std::left << std::setw(18) << "TOTAL" 
              << std::right << std::setw(10) << total_files 
              << std::setw(15) << format_size(total_size) << "\n";
    std::cout << std::string(45, '=') << "\n\n";
}

// --- MAIN CLI ---
int main() {
    std::cout << "Smart File Organizer (CLI)\n";
    std::cout << "==========================\n";

    std::string target_path;
    while (true) {
        std::cout << "\nEnter target folder path (or 'q' to quit): ";
        std::getline(std::cin, target_path);
        target_path = trim(target_path);
        if (target_path == "q" || target_path == "Q") break;
        if (target_path.empty()) continue;

        // Remove surrounding quotes if pasted from terminal
        if (target_path.front() == '"' && target_path.back() == '"') {
            target_path.erase(0, 1);
            target_path.pop_back();
            target_path = trim(target_path);
        }

        fs::path path(target_path);
        if (!fs::exists(path) || !fs::is_directory(path)) {
            std::cout << "❌ Invalid or non-existent folder. Please try again.\n";
            continue;
        }

        std::cout << "✅ Folder selected: " << path.string() << "\n";

        int choice = 0;
        while (true) {
            std::cout << "\n[1] Organize Files\n";
            std::cout << "[2] View Storage Dashboard\n";
            std::cout << "[3] Change Folder\n";
            std::cout << "[4] Exit\n";
            std::cout << "Choose an option: ";

            if (!(std::cin >> choice)) {
                std::cin.clear();
                std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
                std::cout << "❌ Invalid input.\n";
                continue;
            }
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n'); // Clear newline

            if (choice == 1) {
                sort_by_extension(path);
            } else if (choice == 2) {
                show_dashboard(path);
            } else if (choice == 3) {
                break; // Return to folder prompt
            } else if (choice == 4) {
                return 0;
            } else {
                std::cout << "❌ Invalid option. Please try again.\n";
            }
        }
    }

    std::cout << "Exiting. Goodbye!\n";
    return 0;
}