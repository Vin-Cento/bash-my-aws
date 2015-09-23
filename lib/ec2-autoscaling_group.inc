# ec2-autoscaling_group.inc

asg() {
  # type: query
  # returns instance id's and the name tag.
  local inputs=$(__bma_read_inputs $@)

  if __bma_read_switches ${inputs} | grep ^--debug > /dev/null; then
    BMA_DEBUG=true
  else
    BMA_DEBUG=false
  fi

  local default_query='
    AutoScalingGroups[][
      {
        "AutoScalingGroupName": AutoScalingGroupName,
        "Name":       [Tags[?Key==`Name`].Value][0][0]
      }
    ][]
  '

  local asg_names=$(__bma_read_resources $inputs)
  local filters=$(__bma_read_switches $inputs | grep ^--filters | cut -d\  -f2-)
  local query=$(__bma_read_switches $inputs | grep ^--query | cut -d\  -f2-)
  local output=$(__bma_read_switches $inputs | grep ^--output | cut -d\  -f2-)

  if ${BMA_DEBUG}; then
    echo "inputs: ${inputs}"
    echo "asg_names: ${asg_names}"
    echo "filters: ${filters}"
    echo "query: ${query}"
    echo "output: ${output}"
  fi

  [[ -z $query ]] && query=$default_query

  aws autoscaling describe-auto-scaling-groups                    \
    $([[ -n ${asg_names} ]] && echo --auto-scaling-group-names)   \
    $(for x in ${asg_names}; do echo $x; done)                    \
    $([[ -n ${filters} ]] && echo "--filters ${filters}")         \
    --query $(__bma_clean_query ${query})                         \
    --output ${output:-"text"}
}

asg-instances() {
  local inputs=$(__bma_read_inputs $@)
  local asg_names=$(__bma_read_resources ${inputs})
  local filters="
    Name=tag-key,Values=aws:autoscaling:groupName
    Name=tag-value,Values=$(echo ${asg_names} | tr ' ' ',')
  "

  instance                              \
    --filters ${filters}                \
    $(__bma_read_switches ${inputs})
}

asg-capacity() {
  local inputs=$(__bma_read_inputs $@ | base64 -D)
  local query='
    AutoScalingGroups[][
      {
        "AutoScalingGroupName": AutoScalingGroupName,
        "MinSize": MinSize,
        "DesiredCapacity": DesiredCapacity,
        "MaxSize": MaxSize
      }
    ][]
  '

  asg ${inputs} --query ${query}
}

asg-resume() {
  local inputs=$(__bma_read_inputs $@)
  for asg in $(__bma_read_resources ${inputs}); do
    aws autoscaling resume-processes --auto-scaling-group-name $asg
  done
}

asg-suspend() {
  local inputs=$(__bma_read_inputs $@)
  for asg in $(__bma_read_resources ${inputs}); do
    aws autoscaling suspend-processes --auto-scaling-group-name $asg
  done
}

asg-suspended_processes() {
  # TODO: fix the output
  local inputs=$(__bma_read_inputs $@ | base64 -D)
  local query='
    AutoScalingGroups[][
      {
        "AutoScalingGroupName": AutoScalingGroupName,
        "SuspendedProcesses": SuspendedProcesses
      }
    ][]
  '

  asg ${inputs} --query ${query}
}

# vim: ft=sh