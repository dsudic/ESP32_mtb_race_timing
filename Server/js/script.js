// document.getElementById('chip_id').addEventListener('input', function (e) {
//   let target = e.target, position = target.selectionEnd, length = target.value.length;
  
//   target.value = target.value.replace(/\s/g, '').replace(/(\d{5})/g, '$1 ').trim();
//   target.selectionEnd = position += ((target.value.charAt(position - 1) === ' ' && target.value.charAt(length - 1) === ' ' && length !== target.value.length) ? 1 : 0);
// });

function popAlert(msg){
  alert(msg);
}