if [ $# -lt 1 ];
then
    echo "too few argument"
    exit 1
fi
flags=""
command="${!#}"
IFS=" " read -r -a args <<< $command
testName=${args[0]}
echo "test name: $testName"
for a in $@;
do
    if [ $a == "-g" ];
    then
        flags=$flags" --gdb"
    elif [ $a == "-qemu" ];
    then
        flags=$flags" --qemu"
    fi
done

cmd="pintos $flags --filesys-size=2 -p ./build/tests/userprog/$testName -a $testName -- -q -f run '$command'"
echo $cmd
eval $cmd
# pintos --gdb  --filesys-size=2 -p ./build/tests/userprog/flags-single -a flags-single -- -q  -f run 'flags-single onearg'