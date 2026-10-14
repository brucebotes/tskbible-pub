#! /bin/sh

curl -X POST http://localhost:11434/api/chat \
  -H 'Content-Type: application/json' \
  -d '{"stream":false,"messages":[{"role":"system","content":"You are an grammarian and teacher tasked to assist me in understanding the structural parts of sentences. You have access to the 1611 KJV of the bible and are very familiar with the English dialect of that time."},{"role":"user" ,"content":"Given this verse:\nps 1:5 Therefore the ungodly shall not stand in the judgment, nor sinners in the congregation of the righteous.\n"},{"role":"user","content":"Give a proper break down of the phrases and grammar structure of the verse."}],"temperature":0.1,"model":"qwen3:0.6b"}'

