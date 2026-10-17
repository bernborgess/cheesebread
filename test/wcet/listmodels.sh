mapfile -t MODELS < <( opencode models )

PROMPT="HI"
declare -a SUCCESS=()
declare -a FAILS=()

for MODEL in "${MODELS[@]}"; do
  echo "#$MODEL"
  opencode run -m "$MODEL" "$PROMPT"
  if [ $? -ne 0 ]; then
    echo "$MODEL failed."
    FAILS+=($MODEL)
  else
    echo "Success with $MODEL!"
    SUCCESS+=($MODEL)
  fi
done

echo "Fails: ${FAILS[@]}"
echo "Success: ${SUCCESS[@]}"

printf "%s\n" "${SUCCESS[@]}" > models.txt
echo "Successful models stored in models.txt"


