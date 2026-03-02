# Make sure you're on the branch that points to 16f110d
git log --oneline   # confirm the order: 16f110d → 92eeabb → 9d0404a

# Rebase interactively from the grandparent of 92eeabb
git rebase -i 9d0404a
```

In the editor that opens, you'll see:
```
pick 92eeabb init + gpu
pick 16f110d init GPU
```

Change it to:
```
pick 92eeabb init + gpu
squash 16f110d init GPU
```

This will **squash both commits into one**, giving you a single commit with all the changes. Git will prompt you to write a combined commit message — you can just keep `"init GPU"` or whatever you prefer.

**If instead you want to completely drop `92eeabb` and keep only `16f110d`'s changes** (excluding what `92eeabb` added), change the editor to:
```
drop 92eeabb init + gpu
pick 16f110d init GPU
