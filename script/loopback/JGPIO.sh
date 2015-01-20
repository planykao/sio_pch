#!/bin/bash

# Version: 1.0
# Build Date: 09-30-2013



# Check JGPIO


echo "*******************************************"
echo "*     Please install Jumpers in JGPIO     *"
echo "*******************************************"
read -p "Press Enter Key to Continue if Jumpers is install... "

./loopback -c $1

if [ "$?" == "0" ]; then
	echo -e "\033[0;30;42m ************************** \033[0m"
	echo -e "\033[0;30;42m *     Test GPIO Pass     * \033[0m"
	echo -e "\033[0;30;42m ************************** \033[0m"
else
	echo -e "\033[0;30;41m ************************** \033[0m"
	echo -e "\033[0;30;41m *     Test GPIO Fail     * \033[0m"
	echo -e "\033[0;30;41m ************************** \033[0m"
	read -p "Press Enter Key to Continue... "
    echo ""
	exit 1
fi

exit 0
