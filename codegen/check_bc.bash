BT=$1
TARGET=$(echo $BT | cut -d'.' -f 1)
LL="${TARGET}.ll"
O="${TARGET}.o"

llvm-dis $BT -o $LL
llc -filetype=obj $BT -o $O
clang $O -o $TARGET
