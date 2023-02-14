#!/bin/bash
g++ client.cpp -lenet -pthread -o client
g++ lobby.cpp -lenet -o lobby
g++ server.cpp -lenet -o server
