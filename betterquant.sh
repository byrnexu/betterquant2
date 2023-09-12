tar -czvf /tmp/code.tar.gz \
  $(git ls-files | grep -v '\.1' | grep -v 'betterquant\.sh' | grep -v 'sum\.sh')
rm -rf /tmp/betterquant/ && mkdir -p /tmp/betterquant/
tar -xzvf /tmp/code.tar.gz -C /tmp/betterquant/
sed -i '/\/\/!/d' $(grep '//!' /tmp/betterquant/ -rl)
sed -i 's/bitquant2/betterquant/g' $(grep 'bitquant2' /tmp/betterquant/ -rl)
rsync -avzPc /tmp/betterquant/ /mnt/storage/work/betterquant/

cd /mnt/storage/work/betterquant
sed -i 's/DEFAULT_PARALLEL_COMPILE_THREAD_NUM=8/DEFAULT_PARALLEL_COMPILE_THREAD_NUM=1/g' setting.sh
find . -type f -mmin -60 | grep '\.hpp\|\.cpp' | grep -v 3rdparty | \
  grep -v '/build/\|\./inc' | grep -v '\.in$\|\.1$' | xargs -t -i clang-format -i {}
cd -
