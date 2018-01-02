# use whie ... done < "filename" to each time get a line
# use @ and "" to together get a line instead of splitting into word
awk '
    {
        for(i=1; i<=NF; i++)
        {   
            if(line[i] == "")
            {
                line[i] = $i
            }
            else
            {
                line[i] = line[i]" "$i
            }
        }
    }
    END{
         for(i=1; i<=NF; i++)
         {
             print line[i]
         }
       }
    ' file.txt
