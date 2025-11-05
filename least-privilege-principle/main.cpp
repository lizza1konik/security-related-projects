#include <ctime>
#include <iostream>
#include <iomanip>
#include <thread>
#include <unistd.h>
#include <sys/wait.h>
#include <cstring>
#include <vector>
#include <arpa/inet.h>
#include <fstream>
#include <sys/capability.h>
#include <array>
#include <atomic>
#include <algorithm>


#define NETWORK_BUFFER_SIZE 1024
#define MAX_NETWORK_MESSAGE 2048

constexpr const char* BLACKLIST = "%\\";
constexpr std::array<char, 6> MARKERS = {'D', 'M', 'Y', 'h', 'm', 's'};
std::atomic<bool> SERVER_SHUTDOWN {false};

extern int give_up_capabilities(cap_value_t *except, int n)
{
    cap_t caps = cap_get_proc();
    if (caps == NULL)
        return -1;

    if (cap_clear_flag(caps, CAP_PERMITTED) == -1)
    {
        cap_free(caps);
        return -1;
    }
    if (cap_clear_flag(caps, CAP_INHERITABLE) == -1)
    {
        cap_free(caps);
        return -1;
    }
    if (cap_clear_flag(caps, CAP_EFFECTIVE) == -1)
    {
        cap_free(caps);
        return -1;
    }
    if (except != NULL)
    {
        if (cap_set_flag(caps, CAP_PERMITTED, n, except, CAP_SET) == -1)
        {
            cap_free(caps);
            return -1;
        }
        if (cap_set_flag(caps, CAP_EFFECTIVE, n, except, CAP_SET) == -1)
        {
            cap_free(caps);
            return -1;
        }
    }
    if (cap_set_proc(caps) == -1)
    {
        cap_free(caps);
        return -1;
    }
    //printf("Capabilities: %s\n", cap_to_text(caps, NULL));
    if (cap_free(caps) == -1)
        return -1;
    return 0;
}

bool check_user_input(const std::string & buffer)
{
    for (char c : buffer)
    {
        if (strchr(BLACKLIST, c))
            return false;
    }
    bool lastWasMarker = false;
    for (size_t i = 0; i < buffer.size(); ++i)
    {
        if (buffer[i] == '$' &&  i + 1 < buffer.size())
        {
            if (std::find(MARKERS.begin(), MARKERS.end(), buffer[i+1]) != MARKERS.end())
            {
                if (lastWasMarker)
                    return false;
                lastWasMarker = true;
                i++;
            }
        }
        else
        {
            lastWasMarker = false;
        }
    }
    return true;
}

std::string input_instructions()
{
    std::ostringstream out;
    out << "\n====================== INSTRUCTIONS ======================\n";
    out << "This application allows you to:\n";
    out << "1. Display current date and time in a custom format.\n";
    out << "2. Set a new system date and time (only for local users).\n\n";

    out << "------------------ FORMAT COMMANDS ------------------\n";
    out << "You can use the following markers in your input:\n";
    out << "  $D  - day (01–31)\n";
    out << "  $M  - month (01–12)\n";
    out << "  $Y  - year (e.g., 2025)\n";
    out << "  $h  - hour (00–23)\n";
    out << "  $m  - minute (00–59)\n";
    out << "  $s  - second (00–59)\n";
    out << "  Example:  $D.$M.$Y $h:$m:$s\n";
    out << "  → Output:  04.05.2025 13:42:17\n\n";

    out << "-------------------- SET TIME ------------------------\n";
    out << "Local users can set system time with the command:\n";
    out << "  set <dd:mm:yyyy> <hh:mm:ss>\n";
    out << "  Example: set 04:05:2025 13:42:00\n";
    out << "  → This will update the system clock to the provided value.\n";
    out << "  → This feature is NOT available to network clients.\n\n";

    out << "-------------------- QUIT ----------------------------\n";
    out << "To shut down the server and exit the application:\n";
    out << "  QUIT\n";

    out << "========================================================\n\n";

    return out.str();
}

bool check_date_and_time(int & year, int & month, int & day, int & hour, int & minute, int & second)
{
    if ( year < 1970 || month < 1 || month > 12 ||
    day < 1 || day > 31 || hour < 0 || hour > 23 ||
    minute < 0 || minute > 59 || second < 0 || second > 59 )
        return false;

    int days_in_month[] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    if ((year % 4 ==0 && year % 100 != 0) || (year % 400 == 0)) days_in_month[1] = 29;

    if (day > days_in_month[month-1])
        return false;
    return true;
}

void call_set_time(const std::string & user_input)
{
    std::stringstream ss(user_input);
    std::string prefix, date, time;
    if (!(ss >> prefix >> date >> time))
        std::cout << "ERROR: Wrong format! Please try again" << std::endl;

    int year = 0; int month = 0; int day = 0;
    int hour = 0; int minute = 0; int second = 0;
    char d1, d2, d3, d4;

    std::stringstream stream_date(date);
    if (!(stream_date >> day >> d1 >> month >> d2 >> year))
        std::cout << "ERROR: Wrong format! Please try again" << std::endl;

    std::stringstream stream_time(time);
    if (!(stream_time >> hour >> d3 >> minute >> d4 >> second))
        std::cout << "ERROR: Wrong format! Please try again" << std::endl;

    if (!check_date_and_time(year, month, day, hour, minute, second))
    {
        std::cout << "ERROR: Date out of range! Please try again" << std::endl;
        return;
    }

    struct tm t{};
    t.tm_year = year - 1900;
    t.tm_mon = month - 1;
    t.tm_mday = day;
    t.tm_hour = hour;
    t.tm_min = minute;
    t.tm_sec = second;

    time_t timestamp = mktime(&t);
    if (timestamp == -1)
    {
        std::cout << "ERROR: Unable to convert to timestamp" << std::endl;
        return;
    }

    pid_t pid = fork();
    if (pid == 0)
    {
        execl("/usr/local/sbin/set_time", "set_time", std::to_string(timestamp).c_str(), nullptr);
        perror("exec failed");
        exit(1);
    }
    if (pid > 0)
    {
        wait(nullptr);
    }
    else
    {
        perror("fork failed");
    }
}

std::string get_time(const std::string & user_input)
{
    if (!check_user_input(user_input))
        return {"ERROR: Wrong format! Please try again"};


    time_t t = time(nullptr);
    struct tm* now = localtime(&t);
    std::ostringstream out;

    for (size_t i = 0; i < user_input.size(); ++i)
    {
        if (user_input[i] == '$' && i + 1 < user_input.size())
        {
            char marker = user_input[i + 1];
            switch (marker)
            {
                case 'D':
                    out << std::setw(2) << std::setfill('0') << now->tm_mday;
                break;
                case 'M':
                    out << std::setw(2) << std::setfill('0') << (now->tm_mon + 1);
                break;
                case 'Y':
                    out << (now->tm_year + 1900);
                break;
                case 'h':
                    out << std::setw(2) << std::setfill('0') << now->tm_hour;
                break;
                case 'm':
                    out << std::setw(2) << std::setfill('0') << now->tm_min;
                break;
                case 's':
                    out << std::setw(2) << std::setfill('0') << now->tm_sec;
                break;
                default:
                    out << '$' << marker;
                break;
            }
            ++i; // Пропускаем символ-маркер
        }
        else
        {
            out << user_input[i];
        }
    }
    return out.str();
}

void client_manager(const int client_fd)
{
    std::string text_to_send =  "Welcome to Network Time App - Network Part! Enter your request or type QUIT to finish\n" + input_instructions() + "# ";
    send(client_fd, text_to_send.c_str(), static_cast<int>(text_to_send.size()), 0);

    char network_buffer[NETWORK_BUFFER_SIZE];
    std::string messages;
    ssize_t bytes_read = 0;
    while ((bytes_read = recv(client_fd, network_buffer, sizeof(network_buffer), 0)) > 0 && !SERVER_SHUTDOWN)
    {
        messages.append(network_buffer, bytes_read);
        if (messages.size() > MAX_NETWORK_MESSAGE)
        {
            std::string error_text =  "ERROR: Message is too long. Try again!\n";
            send(client_fd, error_text.c_str(), static_cast<int>(error_text.size()), 0);
            break;;
        }

        size_t position;
        while ((position = messages.find('\n')) != std::string::npos)
        {
            std::string current_message = messages.substr(0, position);
            messages.erase(0, position + 1);

            if (check_user_input(current_message))
            {
                std::string text_to_send =  get_time(current_message);
                text_to_send += "\n# ";
                send(client_fd, text_to_send.c_str(), static_cast<int>(text_to_send.size()), 0);
            }
            else
            {
                std::string error_text =  "ERROR: Wrong format! Please try again\n";
                send(client_fd, error_text.c_str(), static_cast<int>(error_text.size()), 0);
            }
        }
    }
    close(client_fd);
}

void network_thread(int port)
{
    sockaddr_in address{};
    int addrlen = sizeof(address);

    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == 0)
    {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, reinterpret_cast<struct sockaddr *>(&address), sizeof(address)) < 0)
    {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, 5) < 0)
    {
        perror("listen failed");
        exit(EXIT_FAILURE);
    }

    while (!SERVER_SHUTDOWN)
    {
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(server_fd, &readfds);
        struct timeval timeout = {1, 0};
        int activity = select(server_fd + 1, &readfds, nullptr, nullptr, &timeout);
        if (activity > 0 && FD_ISSET(server_fd, &readfds))
        {
            int client_fd = accept(server_fd, reinterpret_cast<struct sockaddr *>(&address), reinterpret_cast<socklen_t *>(&addrlen));;
            if (client_fd >= 0)
            {
                std::thread t(client_manager, client_fd);
                t.detach();
            }
        }
    }
    close(server_fd);
}

int define_port(const std::string & file_path)
{
    std::ifstream file(file_path);
    if (!file.is_open())
        throw std::runtime_error("Error opening config file");

    std::string config_text;
    if (!std::getline(file, config_text, '\n'))
        throw std::runtime_error("Error reading config file text");

    const std::string prefix = "PORT=";
    if (config_text.rfind(prefix, 0) != 0)
        throw std::runtime_error("Invalid config format");

    int port = std::stoi(config_text.substr(prefix.size()));
    if (port < 1024 || port > 65535)
        throw std::runtime_error("Invalid port number");

    return port;
}

void cli_thread()
{
    std::cout << "Welcome to Network Time App CLI part! Enter your request or type QUIT to finish" << std::endl;
    std::cout << input_instructions() << std::endl;
    std::string user_input;
    std::cout << "# ";

    while (std::getline(std::cin, user_input))
    {
        if (user_input.rfind("set ", 0) == 0)
        {
           call_set_time(user_input);
        }
        else if (user_input.rfind("QUIT", 0) == 0)
        {
            SERVER_SHUTDOWN = true;
            std::cout << "SHUTDOWN requested!" << std::endl;
            break;
        }
        else
        {
            std::cout << get_time(user_input) << std::endl;
        }
        std::cout << "# ";
    }
}

int main()
{
    give_up_capabilities(nullptr, 0);
    int port = define_port("/etc/network_time.conf");

    std::thread t_cli(cli_thread);
    std::thread t_server(network_thread, port);
    t_cli.join();
    t_server.join();
    return 0;
}
