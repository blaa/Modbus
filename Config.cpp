#include "Config.h"

enum Config::BaudRate Config::BaudRate	= Config::B115200;
enum Config::Parity Config::Parity	= Config::EVEN;
enum Config::StopBits Config::StopBits	= Config::SINGLE;
enum Config::CharSize Config::CharSize	= Config::CS8;
enum Config::Protocol Config::Protocol	= Config::TERMINATED;
