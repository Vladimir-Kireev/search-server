<h1 align="center">Поисковая система</h1>
<h3 align="center"><img src="https://img.shields.io/badge/c++-%2300599C.svg?style=for-the-badge&logo=c%2B%2B&logoColor=white"> <img src="https://img.shields.io/badge/Visual%20Studio-5C2D91.svg?style=for-the-badge&logo=visual-studio&logoColor=white"> <img src="https://img.shields.io/badge/git-%23F05033.svg?style=for-the-badge&logo=git&logoColor=white"></h3>
Search Server - поисковик документов с учетом минус-слов. Принцип работы приближен к существующим поисковикам.
<h3>Основные функции</h3>
<ul>
  <li>Ранжирование результатов поиска по статистической мере TF-IDF;</li>
  <li>Обработка стоп-слов (не учитываются поисковой системой и не влияют на результаты поиска);</li>
  <li>Обработка минус-слов (документы, содержащие минус-слова, не будут включены в результаты поиска);</li>
  <li>Создание и обработка очереди запросов;</li>
  <li>Удаление дубликатов документов;</li>
  <li>Возможность работы в многопоточном режиме.</li>
</ul>
<h3>Системные требования</h3>
<ul>
  <li>C++17</li>
  <li>Clang 15.0.7</li>
</ul>
