BT=$1
TARGET=$(echo $BT | cut -d'.' -f 1)
LL="${TARGET}.ll"
O="${TARGET}.o"

rm $BT
rm $O
rm $LL
rm $TARGET
