detected a still reachable block with 1 free remaining

```
==156043==                                                                                                                                        
==156043== HEAP SUMMARY:
==156043==     in use at exit: 12,912 bytes in 1 blocks
==156043==   total heap usage: 52,911 allocs, 52,910 frees, 48,207,353 bytes allocated
==156043== 
==156043== 12,912 bytes in 1 blocks are still reachable in loss record 1 of 1
==156043==    at 0x484582F: realloc (vg_replace_malloc.c:1437)
==156043==    by 0x109944: edzin_append_line (main.c:265)
==156043==    by 0x10A007: edzin_open (main.c:414)
==156043==    by 0x1092B4: main (main.c:36)
==156043== 
==156043== LEAK SUMMARY:
==156043==    definitely lost: 0 bytes in 0 blocks
==156043==    indirectly lost: 0 bytes in 0 blocks
==156043==      possibly lost: 0 bytes in 0 blocks
==156043==    still reachable: 12,912 bytes in 1 blocks
==156043==         suppressed: 0 bytes in 0 blocks
==156043== 
==156043== ERROR SUMMARY: 0 errors from 0 contexts (suppressed: 0 from 0)
```
