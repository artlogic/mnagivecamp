(function ($) {
  Drupal.Donate = Drupal.Donate || {};

  Drupal.behaviors.donate = {
    attach : function(context) {
      Drupal.Donate.fields = new Array();
      Drupal.Donate.subTotal = new Drupal.Donate.total();

      $(".donate-amount .form-item input").each(function(i) {
        Drupal.Donate.fields[i] = new Drupal.Donate.total();
        Drupal.Donate.fields[i].fieldInit($(this).attr('id'));
      });

      Drupal.Donate.update();
    }
  };

  Drupal.Donate.total = function() {
    this.total = 0;
    this.timeout;
  }

  Drupal.Donate.total.prototype.fieldInit = function(id) {
    this.itemID = '#' + id;
    this.total = $(this.itemID).val();

    var self = this;
    $(self.itemID).keyup(function() {
      var newTotal = Drupal.Donate.formatTotal($(this).val());
      if (self.total != newTotal) {
        self.total = newTotal;
        if (self.timeout) {
          clearTimeout(self.timeout);
        }
        self.timeout = setTimeout("Drupal.Donate.update()", 500);
      }
    });

    $(self.itemID).change(function() {
      $(this).val(self.total);
    });

    $(self.itemID).click(function() {
      $(self.itemID).focus();
      $(self.itemID).select();
    });
  }

  Drupal.Donate.formatTotal = function(total) {
    var newTotal = total;
    if (typeof total == 'string') {
      newTotal = newTotal.replace(/[^0-9.]/g, '');
    }
    newTotal = Math.round(newTotal*100)/100;
    newTotal = newTotal.toFixed(2);
    return newTotal;
  }

  Drupal.Donate.update = function() {
    var total = 0;
    $.each(Drupal.Donate.fields, function(i, field) {
      total = parseFloat(field.total) + total;
    });
    Drupal.Donate.subTotal.total = Drupal.Donate.addCommas(Drupal.Donate.formatTotal(total));

    $('#donate-total span').empty().append(Drupal.Donate.subTotal.total);
    if (total != 0) {
      $('h4#donate-total').highlightFade({
        color: 'yellow',
        speed:2000,
        iterator: 'sinusoidal'
      });
    }
  }

  Drupal.Donate.addCommas = function(nStr) {
    nStr += '';
    x = nStr.split('.');
    x1 = x[0];
    x2 = x.length > 1 ? '.' + x[1] : '';
    var rgx = /(\d+)(\d{3})/;
    while (rgx.test(x1)) {
      x1 = x1.replace(rgx, '$1' + ',' + '$2');
    }
    return x1 + x2;
  }

})(jQuery);