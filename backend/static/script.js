document.addEventListener('DOMContentLoaded', function() {
    const checkboxes = document.querySelectorAll('.camera-toggle');
    checkboxes.forEach(function(checkbox) {
      checkbox.addEventListener('change', function() {
        const camID = checkbox.getAttribute('data-cam');
        const streamElement = document.getElementById('stream-' + camID);
        if (checkbox.checked) {
          streamElement.style.display = 'block';
        } else {
          streamElement.style.display = 'none';
        }
      });
    });
  });