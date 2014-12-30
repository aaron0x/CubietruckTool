#include <sys/inotify.h>
#include <signal.h>
#include <limits.h>
#include <iostream>
#include <string>
#include <cstdlib>
#include <stdexcept>
#include <sstream>
#include <errno.h>
#include <unistd.h>

using namespace std;

bool run = true;

// set values of -d/-f to dir/command.
bool   parseParameters(int argc, char** const argv, char** dir, char** command);

void   subscribeSignals();

// exit program graceful when catching a signal.
void   quitMonitor(int sigNum);

int    initializeInotify(const char* path);

// start to watch dir infinitely. excute command when dir changing.
void   keepMonitorDir(int fd, const char* command);

string&& toStr(int number);

int main(int argc, char** argv) {
   char* dir     = NULL;
   char* command = NULL;

   if (!parseParameters(argc, argv, &dir, &command)) {
      cerr << "Usage: program -d dir_path -c command_for_dir_change\n";
      return EXIT_FAILURE;
   }

   try {
      int fd = initializeInotify(dir);
      subscribeSignals();
      keepMonitorDir(fd, command);
      close(fd);
   } catch (const exception& e) {
      cerr << e.what() << endl;
      return EXIT_FAILURE;  
   }

   return EXIT_SUCCESS;
}

bool parseParameters(int argc, char** const argv, char** dir, char** command) {
   int   o = -1;
   char* d = NULL;
   char* c = NULL;

   while((o = getopt(argc, argv, "d:c:")) != -1) {
      switch(o) {
         case 'd':
            d = optarg;
            break;
         case 'c':
            c = optarg;
            break;
         default:
            return false;
            break;
      }
   }

   if (!d || !c) {
      return false;
   }

   *dir     = d;
   *command = c;

   return true;
}

void subscribeSignals() {
   signal(SIGINT,  quitMonitor);
   signal(SIGHUP,  quitMonitor);
   signal(SIGTERM, quitMonitor);
   signal(SIGKILL, quitMonitor);   
}

void quitMonitor(int sigNum) {
   run = false;
}

int initializeInotify(const char* path) {
   int fd = inotify_init();
   if (fd == -1) {
      throw runtime_error("inotify init failed: " + toStr(errno));
   }

   if (inotify_add_watch(fd, path, IN_MODIFY | IN_CREATE) == -1) {
      close(fd);
      throw runtime_error(string("inotify_add_watch failed ") + path + " " + toStr(errno));
   }

   return fd;
}

void keepMonitorDir(int fd, const char* command) {
   while (run) {
      char buf[sizeof(struct inotify_event) + NAME_MAX + 1] = "";
      int readRc = read(fd, buf, sizeof(buf));
      if (readRc == EIO) {
         throw runtime_error("read inotify fd error");
      }

      // only excute command when real event happened.
      if (readRc > 0) {
         int commandRc = system(command);
         if (commandRc == -1 || WEXITSTATUS(commandRc) != 0) {
            throw runtime_error("command failed, return " + toStr(WEXITSTATUS(commandRc)));
         }   
      }
   }
}

string&& toStr(int number) {
   stringstream ss;
   ss << number;
   return move(ss.str());
}
