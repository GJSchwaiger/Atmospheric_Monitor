#ifndef APP_H
#define APP_H

    #include <iostream>
    #include <fcntl.h>         // For open()
    #include <unistd.h>        // For read(), write(), close(), usleep()
    #include <sys/ioctl.h>     // For ioctl()
    #include <linux/i2c-dev.h> // For I2C definitions
    #include <cstdint>         // For fixed-width integers like uint8_t

    using std::cerr;
    using std::cout;
    using std::endl;

    struct Registers{

    };
#endif