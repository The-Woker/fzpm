#include <cstdlib>
#include <future>
#include <iostream>
#include <thread>
#include <unistd.h>

int main(int argc, char *argv[]) {
    if (argc == 1) {
        std::cerr << "Usage: sudo fzpm -i (install) or -r (remove)\n";
        return 1;
    }

    uid_t euid = geteuid();
    if (euid != 0) {
        std::cerr << "Must be run with root privileges\n";
        return 1;
    }

    std::string arg1 = argv[1];

    std::future<int> apt = std::async(std::launch::deferred, [] {
        return system("command -v apt > /dev/null 2>&1");
    });

    std::future<int> pacman;
    std::future<int> xbps;
    if (apt.get() != 0) {
        pacman = std::async(std::launch::deferred, [] {
            return system("command -v dnf > /dev/null 2>&1");
        });
    } else {
        if (arg1 == "-i") {
            std::system(
                "apt-cache search . | awk '{print $1}' | "
                "fzf --prompt=\"Select package(s) to install: \" --multi "
                "--border | "
                "xargs -ro apt install -y");
            return 0;
        } else if (arg1 == "-r") {
            std::system(
                "apt list --installed 2>/dev/null | cut -d/ -f1 | "
                "fzf --prompt=\"Select package(s) to remove: \" --multi "
                "--border | "
                "xargs -ro apt remove -y");
            return 0;
        } else {
            std::cerr << "Usage: sudo fzpm -i (install) or -r (remove)\n";
            return 1;
        }
    }

    if (pacman.get() != 0) {
        xbps = std::async(std::launch::deferred, [] {
            return system("command -v xbps-install > /dev/null 2>&1");
        });
    } else {
        if (arg1 == "-i") {
            std::system("pacman -Slq | fzf --multi --preview 'pacman -Si {1}' "
                        "| xargs -ro  pacman -S");
            return 0;
        } else if (arg1 == "-r") {
            std::system("pacman -Qq | fzf --multi --preview 'pacman -Qi {1}' | "
                        "xargs -ro pacman -Rns");
            return 0;
        } else {
            std::cerr << "Usage: sudo fzpm -i (install) or -r (remove)\n";
            return 1;
        }
    }

    if (xbps.get() != 0) {
        std::cout << "Apt, pacman and xbps not found" << std::endl;
    } else {
        if (arg1 == "-i") {
            std::system(
                "xbps-query -Rs '*' | awk '{print $2}' | fzf --prompt=\"Select "
                "package(s) to install: \" --multi "
                "--border | xargs -ro xbps-install -Sy");
            return 0;
        } else if (arg1 == "-r") {
            std::system(
                "xbps-query -m | xargs -n1 xbps-uhelper getpkgname | fzf "
                "--prompt=\"Select package(s) to remove: \" --multi "
                "--border | xargs -ro xbps-remove -Ro");
            return 0;
        } else {
            std::cerr << "Usage: sudo fzpm -i (install) or -r (remove)\n";
            return 1;
        }
    }
    return 1;
}
