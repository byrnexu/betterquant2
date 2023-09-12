cat $(find . -type f | grep '\.hpp\|\.cpp' | grep -v 3rdparty | grep -v '/build/\|\./inc' ) | wc -l
