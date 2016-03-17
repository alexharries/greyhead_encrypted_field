/**
 * @file: greyhead-encrypted-field.js
 */

var Drupal = Drupal || {};

(function ($, Drupal) {
  "use strict";

  /**
   * Hide the content of a decrypted .contenteditable until you hover over it.
   */
  Drupal.behaviors.greyheadEncryptedFieldHideContents = {
    attach: function (context, settings) {
      $(".encrypted-field .contenteditable").each(function () {
        var decryptedText = $(this).text();
        var decryptedTextLength = decryptedText.length + 1;
        var stars = Array(decryptedTextLength).join('*');

        $(this).data('decrypted-text', decryptedText);
        $(this).data('stars', stars);
        $(this).text(stars);

        $(this).hover(function () {
          $(this).text($(this).data('decrypted-text'));
        }, function () {
          $(this).text($(this).data('stars'));
        });
      });
    }
  };

  /**
   * Detect clicks on a decrypted encrypted field value's .contenteditable and select
   * all its contents.
   */
  //Drupal.behaviors.greyheadEncryptedFieldSelectContents = {
  //  attach: function (context, settings) {
  //    $(".encrypted-field .contenteditable").click(function() {
  //      window.setTimeout(function() {
  //        var sel, range;
  //        if (window.getSelection && document.createRange) {
  //          range = document.createRange();
  //          range.selectNodeContents(div);
  //          sel = window.getSelection();
  //          sel.removeAllRanges();
  //          sel.addRange(range);
  //        } else if (document.body.createTextRange) {
  //          range = document.body.createTextRange();
  //          range.moveToElementText(div);
  //          range.select();
  //        }
  //      }, 1);
  //    });
  //
  //    //$(".encrypted-field .contenteditable").click(function () {
  //    //  $(this).select();
  //    //});
  //  }
  //};
})
(jQuery, Drupal);
