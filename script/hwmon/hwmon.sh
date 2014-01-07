#!/bin/bash

# Version: 1.0
# Build Date: 09-30-2013



# Check Hardware Monitor

./hwmon 10

if [ "$?" == "0" ]; then
	echo -e "\033[0;30;42m ********************************* \033[0m"
	echo -e "\033[0;30;42m *     Hardware Monitor Pass     * \033[0m"
	echo -e "\033[0;30;42m ********************************* \033[0m"
else
	echo -e "\033[0;30;41m ********************************* \033[0m"
	echo -e "\033[0;30;41m *     Hardware Monitor Fail     * \033[0m"
	echo -e "\033[0;30;41m ********************************* \033[0m"
	read -p "Press Enter Key to Continue... "
    echo ""
	exit 1
fi

exit 0
