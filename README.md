# trec_eval

## Differences

The main differences to the original trec_eval:

1. Add `-p` flag to match query variations. Query `301-1-1` can match
   `301` in qrels.

2. Use hashmap for [performance](#performance) improvement.

3. Accept stdin (specify `-`) as run file input.

4. Support Anserini 3-field format run file.

4. Support `recip_rank_cut`.

## Original README

[README](README)

## Performance

```
$ wc -l qrels.train.tsv queries.train.depth100.trec_run t5.train.queries.prf.ans_run
    484177 qrels.train.tsv
  48413347 queries.train.depth100.trec_run
 106917898 t5.train.queries.prf.ans_run
```

### Original

```
$ trec_eval --version
trec_eval version 9.0.7
```

1. 49m37s

```
$ time trec_eval -m recip_rank qrels.train.tsv queries.train.depth100.trec_run > /dev/null
2970.13user 2.61system 49:37.80elapsed 99%CPU (0avgtext+0avgdata 4588976maxresident)k
219368inputs+0outputs (17major+1372861minor)pagefaults 0swaps
```


### Modified

```
$ trec_eval --version
trec_eval version 9.0.7-lbs-20210321
```

1. 1m5s

```
$ time trec_eval -m recip_rank qrels.train.tsv queries.train.depth100.trec_run > /dev/null
62.88user 2.49system 1:05.82elapsed 99%CPU (0avgtext+0avgdata 4797656maxresident)k
4576208inputs+0outputs (6major+1485574minor)pagefaults 0swaps
```

2. 2m26s

```
$ time trec_eval -p -m recip_rank qrels.train.tsv t5.train.queries.prf.ans_run > /dev/null
130.42user 15.74system 2:26.20elapsed 99%CPU (0avgtext+0avgdata 8319388maxresident)k
0inputs+0outputs (0major+8079924minor)pagefaults 0swaps
```
