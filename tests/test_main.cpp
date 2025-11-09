#include <gtest/gtest.h>
#include <QCoreApplication>
#include <QTimer>

int main(int argc, char **argv) {
    // Initialize Qt application (required for Qt-based tests)
    QCoreApplication app(argc, argv);
    
    // Initialize Google Test
    ::testing::InitGoogleTest(&argc, argv);
    
    // Run tests
    int result = RUN_ALL_TESTS();
    
    return result;
}

