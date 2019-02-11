#!/bin/bash


ARGS="$@"           # script's arguments
ARGC="$#"           # script's arguments count
DEBUG=1             # debug logs mode, see: debug()
filepath="$HOME/bash/task1.out" # default filepath
keepNfiles=''

show_help(){

    #'echo -e'to parse special symbols

    echo -e "\n\ntask1.sh [ -h | --help] \t help menu
task1.sh [ -n [<natural>] ] [<destination>]
    where:
        <natural> \t integer greater then 0
        <destination> \t absolute or local path to file or directory, default is '~/bash/task1.out'
    \n"
}




#check colors available
if [ "$(command tput colors 2>/dev/null )" != "" ];then
    red="$(tput setaf 1)"    
    green="$(tput setaf 2)"
    yellow="$(tput setaf 3)"
    reset="$(tput sgr0)"    

fi



debug(){
    #echo arguments if DEBUG=1
    if [ "$DEBUG" -eq 1 ]; then
        echo "${green}>$@${reset}"
    fi
}

if [ -z $reset ];then #if not exists reset color symbol
    debug 'colors not available'
else
    debug 'colors available'
fi

log(){
    echo "$@" 
}

error(){
    #log arguments to error stream
    echo "${red}Error: $@${reset}" >&2
}

create_path(){
    #creates directory structure and file in it 
    debug "$@"
    mkdir -p `dirname "$@"`
    touch "$@"
}

osinfo(){
    echo "----System----"
    echo "OS Distribution: $( (eval "cat /etc/*-release" | grep 'PRETTY_NAME=' | cut -d '=' -f2) || cat '/etc/issue')"
    echo "Kernel version: $(uname -r)"
    echo "Installation date: $(dumpe2fs -h `mount | grep 'on / ' | cut -d ' ' -f 1` 2>&- | grep 'Filesystem created:' | sed 's/^Filesystem created: //')"
    echo "Hostname: $(uname -n) Uptime: $(uptime | sed 's/^.*up//;s/load average.*$//;s/,[[:blank:]]\{0,\}[[:digit:]]\{1,\}[[:blank:]]users//;s/,//g')"
    echo -e "Processes running : $(ps ax -o pid= | wc -l) \t Users logged in: $(who | wc -l)"

}

hwinfo(){
    echo "----Hardware----"
    echo "CPU: \"$(cat /proc/cpuinfo | grep 'model name' | cut -d ':' -f2- | uniq)\""
    echo "RAM: $(free -m | head -2 | tail -1 | sed "s/[ \t][ \t]*/ /g" | cut -d ' ' -f2) MB"
    echo "Motherboard: \"$(dmidecode -s baseboard-manufacturer)\",\"$(dmidecode -s baseboard-product-name)\" "
    echo "System Serial Number : $(dmidecode -s system-serial-number)"
}

netinfo(){
    echo "----Network----"
    log "$(ip -4 address show | sed -n 's/^[[:digit:]]\{1,\}:[[:blank:]]\{0,\}\([^:]\{1,\}:\).*/\1\t/p;s/[[:blank:]]\{1,\}inet6\{0,1\}[[:blank:]]\(.\{1,\}[[:digit:]]\)[[:blank:]].*/\1/p' | paste -d " " - - )" 
}


allinfo(){
    echo "Date: $( date +'%a,%e %h %Y %T %z' )";
    echo "$(hwinfo)";
    echo "$(osinfo)";
    echo "$(netinfo)";
    echo "---\"EOF\"---"
}

process_arguments(){
    ## parses arguments
    ## calls functions if matched
    if [ -z $# ];then
        error "no arguments provided"
        show_help
        return 1
    fi
    debug 'argc:' $# 'argw:' $@

    if [[ $1 =~ '-(-help)|h)' ]]; then
        show_help  
        return 0 
    fi

    if [ "$1" = '-n' ]; then
        debug '$2 is' $2
        if ! [[ $2 =~ ^[2-9]|([1-9][0-9]+)$ ]]; then
            error "Number should be >= 2"
            return 1
        else
             debug "Number is: $2"
             keepNfiles="$2" ##rewriting global
             shift
             shift
        fi
    fi

    debug  "file args: $*   of length: $#"
    if [ "$#" -gt 1 ]; then
        error "more than one file path specified"
        return 1 
    elif [ "$#" -eq 0 ]; then
        log "file not specified,default is $filepath"
    else
        filepath=$1  ##rewriting global
        debug "file path: $1"
    fi

}


prepare_folder(){

    local filepath="$1"

    local directory="$(dirname $filepath)"
    local datetoday="$(date +'%Y%m%d')"
    debug 'directory' $directory 


    if ! [ -z ${keepNfiles/ /} ] ; then ## if set, delete all old > keepNfiles 
        debug "deleting old, keeping $keepNfiles" 

        #delete all except $filepath, $filepath-$datetoday-<maxnums>

        rm -f `ls -d $directory/* | grep -v -E "^$filepath-$datetoday-[[:digit:]]{4}$" | grep -v -E "^$filepath$" | paste -d " " -` # -v is inverse match # delete all not matched
        rm -f `ls -t -d $directory/* | grep -v -E "^$filepath$" |  head -n -$((keepNfiles-1)) | paste -d " " -`  #delete old
    fi


    local new_name=$filepath 
    if [ -f "$filepath" ]; then
        debug "$filepath exists"

        local name=`ls -dt $directory/* | grep -m 1 -E "^$filepath-$datetoday-[[:digit:]]{4}$" `


        if [ -z "$name" ]; then
            debug "name empty $name"
            new_name="$filepath-$datetoday-0000"
        else 
            debug "name not empty $name"
            local namedate_part=`echo "$name" | sed 's/-[[:digit:]]\{4\}$//'`
            local number_part=`echo "$name" | sed -n 's/^.*-\([[:digit:]]\{4\}\)$/\1/p'`
            debug "nambpat $number_part"
            local number=`echo $number_part | sed 's/^0*//'` ##clear padding zeros

            debug "namba $number"

            number_part="0000$((number+1))" ##pad
            debug "nambpat $number_part"
            number_part=${number_part: -4}  ##fit 4 length
            new_name="$namedate_part-$number_part" ##single return point
        fi

    fi 

    debug 'newname' $new_name

    create_path $new_name
    allinfo > $new_name

}

#    local maxnum=`ls -dr $directory/* | grep -m 1 -E "^$filepath-$datetoday-[[:digit:]]{4}$"  | sed 's/^.*-\([[:digit:]]\{4\}\)$/\1/p'`
##if todayfiles empty 

#    debug "maxnum $maxnum"
#
#        #todo: if <maxnum> exists then rename increasename <maxnum>
#        #todo: else rename 
#        #todo: delete <minnum> file
## minnum is while 
#
#        debug 'file already exists'
#        
#        increasename(){
#            ##increasing numberpart algorithm
#set -x
#
#            local namedate_part=`echo $1 | sed 's/-[[:digit:]]\{4\}$//'`
#            local number_part=`echo $1 | sed -n 's/^.*-\([[:digit:]]\{4\}\)$/\1/p'`
#            ##obsolete: #number_part=${number_part/ /} ##clear possible spaces
#            if [ -z "$number_part" ]; then
#                number_part='0000'
#            else 
#                local number=`echo $number_part | sed 's/^0*//'` ##clear padding zeros
#                number_part="0000$((number+1))" ##pad
#                number_part=${number_part: -4}  ##fit 4 length
#            fi
#            echo "$namedate_part-$number_part" ##single return point
#set +x
#        }
#
#        rename(){
#        set -x
#            ##$1 p_from 
#            ##$2 p_to
#            local renameto = "$2"
#            while [ -f "$renameto" ]
#            do    
#                renameto = `increasename $2`  
#            done
#
#            mv $1 $2    ##point out of recursion
#           debug 'renamed'$1 $2
#        set +x
#        }
#
#        rename $filepath `increasename "$filepath-$datetoday"`
#    
#    fi
#    


#    if ! [ -z ${keepNfiles/ /} ] ; then ## if set, delete all old and > keepNfiles new
#        debug "deleting old, keeping $keepNfiles" 
#        
#        #todo: delete all except $filepath, $filepath$datetoday-<maxnums>
#
#        rm -f `ls -d $directory/* | grep -v -E "^$filepath$datetoday-[[:digit:]]{4}$" | grep -v -E "^$filepath$" | paste -d " " -` # -v is inverse match
#        rm -f `ls -dr $directory/* | head -n -$keepNfiles | paste -d " " -`
#    fi
#}

set -e
set -o pipefail
process_arguments $ARGS 
prepare_folder $filepath
debug "end script"


