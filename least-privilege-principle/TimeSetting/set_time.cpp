#include <iostream>
#include <cstring>
#include <ctime>
#include <sys/time.h>
#include <sys/capability.h>
using std::stoi;

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

int set_effective_cap(cap_value_t *caps_to_set, int n, bool enable)
{
    if (caps_to_set == nullptr || n <= 0)
        return -1;

    cap_t caps = cap_get_proc();
    if (caps == nullptr)
        return -1;

    cap_flag_value_t flag = enable ? CAP_SET : CAP_CLEAR;
    if (cap_set_flag(caps, CAP_EFFECTIVE, n, caps_to_set, flag) == -1)
    {
        cap_free(caps);
        return -1;
    }

    if (cap_set_proc(caps) == -1)
    {
        cap_free(caps);
        return -1;
    }

    cap_free(caps);
    return 0;
}

int main(int argc, char* argv[])
{
    cap_value_t SYS_TIME[] = { CAP_SYS_TIME };
    give_up_capabilities(SYS_TIME, 1);
    set_effective_cap(SYS_TIME, 1, false);

    if (argc != 2)
        return 1;

    char* end = nullptr;
    errno = 0;
    time_t timeSinceEpoch = strtoll(argv[1], &end, 10);
    if (!*argv[1] || *end || errno)
        return 1;

    struct timeval tv;
    tv.tv_sec = timeSinceEpoch;
    tv.tv_usec = 0;

    set_effective_cap(SYS_TIME, 1, true);
    if (settimeofday(&tv, nullptr) == -1)
        return 1;
    give_up_capabilities(nullptr, 0);

    return 0;
}
