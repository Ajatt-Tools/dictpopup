#!/bin/bash

# TODO: 
# - Store created files in tmp dir

assert_equals() {
  if [[ "$1" == "$2" ]]; then
      echo "Test passed"
      return 0
  else
      echo "Test failed!"
      vimdiff  <(echo "$1") <(echo "$2")
      return 1
  fi
}

cd "${0%/*}"/..
curdir="$(pwd)"
echo "Working directory: $curdir"

trap "rm -f ./tests/files/testdict.zip ./tests/files/data.mdb" EXIT

echo "Zipping test dictionary..."
zip -j -r ./tests/files/testdict.zip ./tests/files/testdictionary/*

echo "Creating index..."
dictpopup-create -d ./tests/files/ -i ./tests/files/

output=$(DICTPOPUP_CONFIG_DIR="$curdir" ./dictpopup-cli -d ./tests/files/ "面白い")

expected_output=$'dictname: Test dictionary
kanji: 面白い
reading: おもしろい
definition: おもしろ・い[4]【面白い】
（形）
(ク)おもしろ・し
〔「面(おも)白し」で、目の前がぱっと明るくなる感じを表すのが原義といわれる〕
①楽しい。愉快だ。
「昨日見た映画は━・かった」
「勉強が━・くて仕方がない」
②興味をそそる。興味深い。
「何か━・い話はないか」
「最後にきて━・い展開を見せる」
③こっけいだ。おかしい。
「━・いしぐさで人を笑わせる」
④（多く、打ち消しの語を伴う）心にかなう。好ましい。望ましい。
「病状が━・くない」
「━・くない結果に終わる」
「私に━・からぬ感情を抱いている」
⑤景色などが明るく広々とした感じで、気分がはればれとするようだ。明るく目が覚めるようだ。
「十日あまりなれば、月━・し／土左日記」
⑥心をひかれる。趣が深い。風流だ。
「昔を思ひやりてみれば━・かりける所なり／土左日記」
〔類義の語に「おかしい」があるが、「おかしい」は格好・表情・しぐさ・話し方などが普通と違っていて、笑いたくなる意を表す。それに対して「おもしろい」は対象が普通の基準から見ると新鮮・奇抜で変化に富んでいて、興味をそそる意を表す〕
━が・る（動(ラ)五［四］）・━げ（形動）・━さ（名）・━み（名）

dictname: Test dictionary
kanji: 面白い
reading: おもしろい
definition: ①  
\t• interesting
\t• fascinating
\t• intriguing
\t• enthralling
\t◦ 新聞には何も面白いことは載っていない。
\t   There is nothing interesting in the newspaper.
②  
\t• amusing
\t• funny
\t• comical
\t◦ これは私が読んだ中で一番面白い本です。
\t   This is the funniest book in my reading.
\t◦ 彼女は子供たちに面白い話をしてあげた。
\t   She told her children an amusing story.
③  
\t• enjoyable
\t• fun
\t• entertaining
\t• pleasant
\t• agreeable
\t◦ この本は面白い読み物です。
\t   This book makes pleasant reading.
\t◦ 野鳥を観察するのはとても面白い。
\t   Watching wild birds is great fun.
④  
\t• good
\t• satisfactory
\t• favourable
\t• desirable
\t• encouraging
\t📝  usu. in the negative

dictname: Test dictionary
kanji: 面白い
reading: おもしろい
definition: おも‐しろ・い【×面白い】（形）《カロ・カツ（ク）・イ・イ・ケレ・○》
① 満足できる内容であり心ひかれる。愉快だ。楽しい。「あの人は―」
② 発展が期待でき興味深い。「なかなか―意見だ」
③ 笑い出したくなる。おかしい。こっけいだ。
④ 好ましい。「どうも結果が―・くない」
〔文〕おもしろ・し（ク）
「面（おも）」は目前の情景で、それが広々とひらける意から。
④は、多くあとに打ち消しの語を伴う。

dictname: Test dictionary
kanji: 面白い
reading: おもしろい
definition: おもしろい【面白い】面白い／愉快／痛快
心が楽しく、おかしく、気持ちが晴れるようなさま。
📚使い方
◦ 面白い 【形】 ▽面白いように魚が釣れた ▽その学説は面白い
◦ 愉快 【名・形動】 ▽毎日愉快に過ごしている ▽愉快な仲間
◦ 痛快 【名・形動】 ▽彼の発言は痛快だった ▽逆転勝ちした痛快な試合
🔄使い分け
１ 「面白い」は、広い意味を持つ語。喜ばしく、心がひかれるさまをいう。おかしいの意味のときは「愉快」に重なり、胸がすくの意味では「痛快」に重なる。また、一風変わっていたり新鮮であったりして、興味をひかれるさまにもいうが、この意味では他の二語とは重ならない。
２ 「愉快」は、おかしくて笑いを誘うようなさまをいう。
３ 「痛快」は、楽しく、胸の晴れるさま、胸のつかえが取り払われてすっとするようなさまをいう。また、単に豪快で気分のよいさまなどにも用いる。「痛快な飲みっぷり」「痛快な男」'

assert_equals "$output" "$expected_output"
