#/bin/sh

var=$(cat words.txt)
echo $var | tr ' ' '\n' | sort  | uniq -c | sort -nrk 2 | awk '{print $1 " " $2}'
