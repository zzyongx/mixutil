#include <unistd.h>
#include <gtest/gtest.h>
#include <mixutil/logger.h>

std::auto_ptr<MixUtil::Logger> logger;

TEST(LoggerTEST, test) {
    logger.reset(new MixUtil::Logger("x.txt"));

    logger->log(MixUtil::Logger::LOG_FATAL, EACCES, "access deny %d + %d = %d", 1, 2, 3);
    log_error(0, "access deny %s", "x");

    FILE *fp = fopen("x.txt", "r");
    char buffer[128];
    while (fgets(buffer, 128, fp)) {
        printf(buffer);
    }
    fclose(fp);

    unlink("x.txt");
}
