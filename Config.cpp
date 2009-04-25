#include "Config.h"

enum Config::BaudRate Config::BaudRate	= Config::BR115200;
enum Config::Parity Config::Parity	= Config::EVEN;
enum Config::StopBits Config::StopBits	= Config::SINGLE;
enum Config::CharSize Config::CharSize	= Config::CharSize8;

enum Config::Protocol Config::Protocol	= Config::TERMINATED;
enum Config::Role Config::Role		= Config::MASTER;
unsigned char Config::Address		= 1; 

