# Sample data

Hand-crafted inputs — **not** original PA2 test files.

## Zone graph (`base.txt`)

```
gate ──5──► headquarters ──10──► lab
                 │                 │
                 8                 3
                 ▼                 ▼
              armory ◄─────────────┘
```

Travel times are in minutes. All edges are bidirectional.

## Log file (`sample.log`)

TEXT-format blocks with three people:

| Person | Entry zone | Entry time | Exit time |
|---|---|---|---|
| Alice Cooper | gate | 08:00 | 09:30 |
| Bob Smith | gate | 08:10 | 08:40 |
| Carol White | lab | 08:35 | 09:10 |

## Expected output

```
Who could have visited 'armory' (any time)?
  · Alice Cooper    [gate→armory = 5+8 = 13 min → window 08:13–09:17]
  · Bob Smith       [gate→armory = 13 min → window 08:23–08:27]
  · Carol White     [lab→armory  = 3 min  → window 08:38–09:07]

Who could have visited 'armory' between 08:20–08:30?
  · Alice Cooper    [window 08:13–09:17 overlaps 08:20–08:30 ✓]
  · Bob Smith       [window 08:23–08:27 overlaps 08:20–08:30 ✓]
  (Carol excluded — earliest possible arrival 08:38 > 08:30)

Who could have visited 'lab' (any time)?
  · Alice Cooper    [gate→lab = 5+10 = 15 min → window 08:15–09:15]
  · Bob Smith       [gate→lab = 15 min → window 08:25–08:25]
  · Carol White     [lab→lab  = 0 min  → window 08:35–09:10]
```
