list=`grep -r "include" . | grep "<boost/.*/.*.hpp" | awk -F'<boost/' '{print $2}' | awk -F'/' '{print $1}' | sort -u`
echo $list

for i in $list
do
	if [ $i = '.' ] || [ $i = '..' ];then
		echo "$i == ."
	else
		cp -r /home/xue/caffe_depend/boost_1_69_0/boost/$i ./
	fi
done
