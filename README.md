# Tri

## A toy BF derivative written in c++ for fun.

### Introduction

I'll assume you're already familiar with the esoteric BF language, in case you're not here's what's included:

| Operator  | Description                                                                                                                                                                        |
| --------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| >         | increment the data pointer (to point to the next cell to the right).                                                                                                               |
| <         | decrement the data pointer (to point to the next cell to the left).                                                                                                                |
| +         | increment (increase by one) the byte at the data pointer.                                                                                                                          |
| -         | decrement (decrease by one) the byte at the data pointer.                                                                                                                          |
| .         | output the byte at the data pointer.                                                                                                                                               |
| ,         | decrement (decrease by one) the byte at the data pointer.                                                                                                                          |
| [         | if the byte at the data pointer is zero, then instead of moving the instruction pointer forward to the next command, jump it forward to the command after the matching ] command.  |
| ]         | if the byte at the data pointer is nonzero, then instead of moving the instruction pointer forward to the next command, jump it back to the command after the matching [ command.  |

Tri further expands on BF, here's currently what's added:

| Operator  | Description                                                                                                                                                                        |
| --------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| ?         | output the integer value at the data pointer (as opposed to writing it directly like `.`)                                                                                          |
| /         | Enter "no comment" mode where every character is reserved as a potential operator and comments are not allowed to be written                                                       |
| /         | Exit "no coment" mode (if already enabled.)                                                                                                                                        |

"no comment" mode commands:

| Operator  | Description                                                                                                                                                                        |
| --------- | ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------- |
| $         | increment the data pointer with the current cell (`ptr += tape[ptr]`)                                                                                                              |
