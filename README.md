# trec_eval

## Differences

The main differences to the original trec_eval:

1. Add -p flag to match query variations. Query 301-1-1 can match 301
   in qrels.

2. Use hashmap to **boost performance**. The time evaluating a big run
   file of 48413347 lines (2.2GB) against a qrels file of 484177 lines
   reduces from 40 minutes to 1 minute.

3. Accept stdin (specify "-") as run file input.

## Original README

[README](README)
