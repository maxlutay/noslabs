
cd $PWD

SRCIMAGE=
FORMAT=
SIZE=



read -p 'Enter image path:' SRCIMAGE

set -x
while ! [ -f "$SRCIMAGE" ]; do
    read -p 'Enter image path:' SRCIMAGE
echo "$SRCIMAGE"
done
set +x

read -p 'Enter format:' FORMAT
if ! [ -z "${FORMAT/ /}" ]; then
    FORMAT="-format $FORMAT"
fi

SIZE=`read -p 'Enter size:'`
if ! [ -z "${SIZE/ /}" ]; then
    SIZE="-size $SIZE"
fi


mogrify  $FORMAT $SIZE "${SRCIMAGE/ /}"



exit









