pid_list=$(ps -ef|grep -i './bq' | grep -v grep | awk '{print $2}' | xargs | sed 's/ /,/g')
echo $pid_list
if [[ ! -z $pid_list ]]; then
  echo top -p $pid_list
  top -p $pid_list
fi
