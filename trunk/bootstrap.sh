echo "Generating test.txt file..."
dd if=/dev/random of=test.txt bs=1024 count=10240 >/dev/null 2>&1
echo "All done!"
