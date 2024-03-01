# facade
This is my personal website running on the [drogon](https://github.com/drogonframework/drogon) web framework, which contains:
* memos - a [hugo-moments](https://github.com/FarseaSH/hugo-theme-moments) static micro blog.
* blog - a [zola-serene](https://github.com/isunjn/serene) static blog.
* dailyhot - a web crawler with a [frontend](https://github.com/shankun/dailyhot) using Vue.js.

The gcc compiler support c++20 is needed to build this project.

Dependencies:
1. [tree-sitter](https://github.com/tree-sitter/tree-sitter)
2. [tree-sitter-html](https://github.com/tree-sitter/tree-sitter-html)
3. [Beautiful-Soup-CPP](https://github.com/shankun/Beautiful-Soup-CPP)  to parse HTML document.
4. [pugixml](https://github.com/zeux/pugixml)  to parse xml document.
5. [Iconv](https://www.gnu.org/software/libiconv/)  to convert GB18030 to UTF-8.

TODO list:
* 网易云音乐歌榜
* QQ音乐歌榜
* 今日诗词
* 定时抓取存入postgresql
