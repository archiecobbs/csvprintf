FLAGS=( -n '%[1]s:%1$s\n' )
STDIN='"aaa","bbb"\n"a1","b1","b2"\n'
STDOUT=$'aaa:a1\n'
STDERR=''
EXITVAL='0'
