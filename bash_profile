# .bash_profile

# Get the aliases and functions
if [ -f ~/.bashrc ]; then
        . ~/.bashrc
fi

# User specific environment and startup programs

PATH=$PATH:$HOME/bin
ENV=$HOME/.bashrc
USERNAME="root"
PRINTER="your_printer_name_under_printtool"

export USERNAME ENV PATH PRINTER

mesg n
