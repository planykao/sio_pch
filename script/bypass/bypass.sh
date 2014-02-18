#!/bin/bash

# Version: 1.0
# Build Date: 02-07-2014



# Bypass

./bypass $1 $2 $3 $4 $5

if [ "$?" == "0" ]; then
	echo -e "\033[0;30;42m ********************************* \033[0m"
	echo -e "\033[0;30;42m *           Bypass Pass         * \033[0m"
	echo -e "\033[0;30;42m ********************************* \033[0m"
else
	echo -e "\033[0;30;41m ********************************* \033[0m"
	echo -e "\033[0;30;41m *           Bypass Fail         * \033[0m"
	echo -e "\033[0;30;41m ********************************* \033[0m"
	read -p "Press Enter Key to Continue... "
    echo ""
	exit 1
fi

exit 0
