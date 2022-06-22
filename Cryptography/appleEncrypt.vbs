set words_to_encrypt to “I want to encrypt sentences. Not just words!”
set multiplier to 5
set charList to {“a”, “b”, “c”, “d”, “e”, “f”, “g”, “h”, “i”, “j”, “k”, “l”, “m”, “n”, “o”, “p”, “q”, “r”, “s”, “t”, “u”, “v”, “w”, “x”, “y”, “z”,”A”, “B”, “C”, “D”, “E”, “F”, “G”, “H”, “I”, “J”, “K”, “L”, “M”, “N”, “O”, “P”, “Q”, “R”, “S”, “T”, “U”, “V”, “W”, “X”, “Y”, “Z”, “1”, “2”, “3”,”4″, “5”, “6”, “7”, “8”, “9”, “0”, ” “, “~”, “!”, “@”, “#”, “$”, “%”, “^”, “&”, “*”, “(“, “)”, “_”, “+”, “{“, “}”, “|”, “:”, “\””, “<“, “>”, “?”, “`”, “-“,”=”, “[“, “]”, “\\”, “;”, “‘”, “,”, “.”, “/”, “a”}considering case
–get a list of numbers for the words corresponding to the item numbers of the characters in charList
set p_letter_list to text items of words_to_encrypt
set p_num_list to {}
repeat with i from 1 to (count of p_letter_list)
set this_letter to item i of p_letter_list
repeat with j from 1 to count of charList
set this_char to item j of charList
if this_letter is this_char then
set end of p_num_list to j
exit repeat
end if
end repeat
end repeat
–encrypt the numbers
set modulus to count of charList
set c_num_list to {}
repeat with i from 1 to (count of p_num_list)
set p_num to item i of p_num_list
set c_num to ((p_num * multiplier) mod modulus)
set end of c_num_list to c_num
end repeat
–get the characters for the encrypted numbers corresponding to the characters in charList
set c_letter_list to {}
repeat with i from 1 to (count of c_num_list)
set this_item to item i of c_num_list
set end of c_letter_list to (item this_item of charList)
end repeat
–coerce the encrypted characters into a string
set c_string to c_letter_list as string
end considering
